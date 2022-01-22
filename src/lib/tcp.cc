#include "cppev/tcp.h"

namespace cppev {

void iohandler::async_write(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size()) {
        dp->on_write_complete(iop, evp);
    } else {
        evp->fd_remove(iop, false);
        evp->fd_register(iop, fd_event::fd_writable);
    }
}

void iohandler::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->read_all(sysconfig::buffer_io_step);
    dp->on_read_complete(iop, evp);
    iopt->rbuf()->clear();
    if (iopt->eof() || iopt->is_reset()) {
        dp->on_closed(iop, evp);
        evp->fd_remove(iop, true);
    }
}

void iohandler::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size()) {
        evp->fd_remove(iop, false);
        dp->on_write_complete(iop, evp);
        evp->fd_register(iop, fd_event::fd_readable);
    }
    if (iopt->eop() || iopt->is_reset()) {
        dp->on_closed(iop, evp);
        evp->fd_remove(iop, true);
    }
}


void acceptor::listen(int port, family f, const char *ip) {
    sock_ = nio_factory::get_nsocktcp(f);
    sock_->listen(port, ip);
}

void acceptor::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    std::vector<std::shared_ptr<nsocktcp> > conns = iopt->accept();
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());

    for (auto &p : conns) {
        log::info << "new fd " << p->fd() << " accepted" << log::endl;
        event_loop *io_evlp = d->minloads_get_evlp();
        io_evlp->fd_register(std::dynamic_pointer_cast<nio>(p),
            fd_event::fd_writable, acceptor::on_writable, true);
    }
}

void acceptor::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    evp->fd_remove(iop, true);
    // The sequence CANNOT be changed, since on_accept may call async_write
    evp->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_accept(iop, evp);
    evp->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void acceptor::run_impl() {
    evp_->fd_register(std::dynamic_pointer_cast<nio>(sock_),
        fd_event::fd_readable, acceptor::on_readable, true);
    evp_->loop();
}

void connector::add(std::string ip, int port, family f, int t) {
    tp_shared_data *d = static_cast<tp_shared_data *>(evp_->data());
    std::unique_lock<std::mutex> lock(d->lock);
    auto h = std::make_tuple<>(ip, port, f);
    if (d->hosts.count(h)) { d->hosts[h] += t; }
    else { d->hosts[h] = t; }
    wrp_->wbuf()->put("0");
    wrp_->write_all(1);
}

void connector::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nstream *rdsp = dynamic_cast<nstream *>(iop.get());
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    rdsp->read_all(1);
    std::unique_lock<std::mutex> lock(d->lock);
    for (auto iter = d->hosts.begin(); iter != d->hosts.end(); ) {
        for (int i = 0; i < iter->second; ++i) {
            std::shared_ptr<nsocktcp> sock = nio_factory::get_nsocktcp
                (std::get<2>(iter->first));
            sock->connect(std::get<0>(iter->first), std::get<1>(iter->first));
            event_loop *io_evlp = d->minloads_get_evlp();
            io_evlp->fd_register(std::dynamic_pointer_cast<nio>(sock),
                fd_event::fd_writable, connector::on_writable, true);
        }
        iter = d->hosts.erase(iter);
    }
}

void connector::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    evp->fd_remove(iop, true);      // remove previous callback
    if (!iopt->check_connect()) {
        std::tuple<std::string, int, family> h = iopt->connpeer();
        log::error << "connect failed with " << std::get<0>(h) << " "
            << std::get<1>(h) << log::endl;
        if (d->failures.count(h)) { d->failures[h] += 1; }
        else { d->failures[h] = 1; }
        return;
    }
    // The sequence CANNOT be changed since on_connect may call aysnc_write
    evp->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_connect(iop, evp);
    evp->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void connector::run_impl() {
    evp_->fd_register(std::dynamic_pointer_cast<nio>(rdp_),
        fd_event::fd_readable, connector::on_readable, true);
    evp_->loop();
}

}

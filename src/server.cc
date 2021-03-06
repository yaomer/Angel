#include <signal.h>
#include <stdlib.h>

#include "server.h"

using namespace angel;

server::server(evloop *loop, inet_addr listen_addr)
    : loop(loop),
    listener(new listener_t(
                loop, listen_addr, [this](int fd){ this->new_connection(fd); })),
    conn_id(1),
    ttl_ms(0),
    high_water_mark(0)
{
    // if (is_set_cpu_affinity)
        // util::set_thread_affinity(pthread_self(), 0);
}

evloop *server::get_next_loop()
{
    if (io_thread_pool && io_thread_pool->threads() > 0)
        return io_thread_pool->get_next_loop();
    else
        return loop;
}

void server::new_connection(int fd)
{
    size_t id = conn_id++;
    evloop *io_loop = get_next_loop();
    connection_ptr conn(new connection(id, io_loop, fd));
    conn->set_connection_handler(connection_handler);
    conn->set_message_handler(message_handler);
    conn->set_write_complete_handler(write_complete_handler);
    conn->set_high_water_mark_handler(high_water_mark, high_water_mark_handler);
    conn->set_close_handler([this](const connection_ptr& conn){
            this->remove_connection(conn);
            });
    connection_map.emplace(id, conn);
    set_ttl_timer_if_needed(io_loop, conn);
    io_loop->run_in_loop([conn]{ conn->establish(); });
}

void server::remove_connection(const connection_ptr& conn)
{
    // 这里禁用conn的TTL，因为如果用户开启了TTL，并且在close_handler()中发送了数据，
    // 而conn->send()中会更新连接的ttl timer，从而会导致conn的生命期被延长，
    // 即再经过ttl时间后才会被关闭
    conn->set_ttl(0, 0);
    if (close_handler) close_handler(conn);
    conn->set_state(connection::state::closed);
    conn->get_loop()->remove_channel(conn->get_channel());
    // 必须在主线程的evloop中移除一个连接，否则多个线程就有可能并发
    // 修改connection_map
    // 例如，主线程接收到一个新连接，之后调用new_connection将它添加到
    // connection_map中，如果此时恰好有某个io子线程要移除一个连接，并且
    // 正在调用remove_connection，这时两个线程就会同时修改connection_map，
    // 这会导致难以预料的后果
    loop->run_in_loop([this, id = conn->id()]{
            this->connection_map.erase(id);
            });
}

void server::set_ttl_timer_if_needed(evloop *loop, const connection_ptr& conn)
{
    if (ttl_ms <= 0) return;
    size_t id = loop->run_after(ttl_ms, [conn]{ conn->close(); });
    conn->set_ttl(id, ttl_ms);
}

void server::clean_up()
{
    if (exit_handler) exit_handler();
    log_info("server ready to exit...");
    __logger.quit();
    fprintf(stderr, "\n");
    quit();
}

void server::start()
{
    log_info("server %s is running", listener->addr().to_host());
    // 必须忽略SIGPIPE信号，不然当向一个已关闭的连接发送消息时，会导致服务端意外退出
    add_signal(SIGPIPE, nullptr);
    add_signal(SIGINT, [this]{ this->clean_up(); });
    add_signal(SIGTERM, [this]{ this->clean_up(); });
    listener->listen();
}

void server::daemon()
{
    __logger.quit();
    util::daemon();
    __logger.restart();
}

void server::quit()
{
    loop->quit();
}

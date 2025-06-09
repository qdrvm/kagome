#pragma once

// Макросы для отслеживания shared_ptr<YamuxedConnection>
// Использование: включить этот заголовок в код kagome и использовать макросы

#ifdef YAMUX_SHARED_PTR_TRACKING

extern "C" {
    void __yamux_shared_ptr_created(void* shared_ptr_addr, void* yamux_object);
    void __yamux_shared_ptr_copied(void* new_shared_ptr_addr, void* old_shared_ptr_addr);
    void __yamux_shared_ptr_destroyed(void* shared_ptr_addr);
}

// Макрос для отслеживания создания shared_ptr<YamuxedConnection>
#define YAMUX_TRACK_SHARED_PTR_CREATE(shared_ptr_var, yamux_object) \
    do { \
        __yamux_shared_ptr_created((void*)&(shared_ptr_var), (void*)(yamux_object)); \
    } while(0)

// Макрос для отслеживания копирования shared_ptr<YamuxedConnection>
#define YAMUX_TRACK_SHARED_PTR_COPY(new_shared_ptr, old_shared_ptr) \
    do { \
        __yamux_shared_ptr_copied((void*)&(new_shared_ptr), (void*)&(old_shared_ptr)); \
    } while(0)

// Макрос для отслеживания уничтожения shared_ptr<YamuxedConnection>
#define YAMUX_TRACK_SHARED_PTR_DESTROY(shared_ptr_var) \
    do { \
        __yamux_shared_ptr_destroyed((void*)&(shared_ptr_var)); \
    } while(0)

// Вспомогательный макрос для автоматического отслеживания shared_ptr в конструкции
#define YAMUX_TRACKED_SHARED_PTR(var_name, yamux_connection_ptr) \
    std::shared_ptr<libp2p::connection::YamuxedConnection> var_name(yamux_connection_ptr); \
    YAMUX_TRACK_SHARED_PTR_CREATE(var_name, yamux_connection_ptr)

#else

// Когда трекинг отключен - макросы ничего не делают
#define YAMUX_TRACK_SHARED_PTR_CREATE(shared_ptr_var, yamux_object) do {} while(0)
#define YAMUX_TRACK_SHARED_PTR_COPY(new_shared_ptr, old_shared_ptr) do {} while(0)
#define YAMUX_TRACK_SHARED_PTR_DESTROY(shared_ptr_var) do {} while(0)
#define YAMUX_TRACKED_SHARED_PTR(var_name, yamux_connection_ptr) \
    std::shared_ptr<libp2p::connection::YamuxedConnection> var_name(yamux_connection_ptr)

#endif

// Примеры использования в коде kagome:
/*

// 1. В местах создания shared_ptr<YamuxedConnection>:
auto connection = std::make_shared<YamuxedConnection>(...);
YAMUX_TRACK_SHARED_PTR_CREATE(connection, connection.get());

// 2. В местах копирования shared_ptr:
auto connection_copy = connection;  // обычное копирование
YAMUX_TRACK_SHARED_PTR_COPY(connection_copy, connection);

// 3. В контейнерах:
connections_.push_back(connection);
YAMUX_TRACK_SHARED_PTR_COPY(connections_.back(), connection);

// 4. В функциях с параметрами shared_ptr:
void storeConnection(std::shared_ptr<YamuxedConnection> conn) {
    stored_connection_ = conn;
    YAMUX_TRACK_SHARED_PTR_COPY(stored_connection_, conn);
}

// 5. Автоматическое отслеживание:
YAMUX_TRACKED_SHARED_PTR(my_connection, std::make_shared<YamuxedConnection>(...));

*/ 
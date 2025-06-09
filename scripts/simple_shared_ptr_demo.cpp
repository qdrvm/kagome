#include <iostream>
#include <memory>
#include <vector>
#include <map>

// Простая структура для отслеживания
static std::map<void*, std::string> tracked_objects;

// Тестовый класс YamuxedConnection
class YamuxedConnection {
private:
    std::vector<char> data;
    int id;
    static int next_id;

public:
    YamuxedConnection() : data(400, 'X'), id(++next_id) {
        std::cout << "🏗️ YamuxedConnection #" << id << " создан (ptr: " << this << ")\n";
        tracked_objects[this] = "YamuxedConnection #" + std::to_string(id);
    }
    
    ~YamuxedConnection() {
        std::cout << "💀 YamuxedConnection #" << id << " уничтожен (ptr: " << this << ")\n";
        tracked_objects.erase(this);
    }
    
    int getId() const { return id; }
};

int YamuxedConnection::next_id = 0;

// Глобальные переменные для имитации утечек
std::vector<std::shared_ptr<YamuxedConnection>> global_connections;
static std::shared_ptr<YamuxedConnection> static_connection;

// Функция для логирования состояния shared_ptr
void log_shared_ptr_info(const std::shared_ptr<YamuxedConnection>& ptr, const std::string& context) {
    if (ptr) {
        std::cout << "🔗 " << context << ": ptr=" << ptr.get() 
                  << ", ref_count=" << ptr.use_count() << "\n";
    }
}

// Функция, создающая утечку через глобальную переменную  
void create_global_leak() {
    std::cout << "\n2️⃣ === Создаем утечку через глобальную переменную ===\n";
    auto conn = std::make_shared<YamuxedConnection>();
    log_shared_ptr_info(conn, "После создания");
    
    global_connections.push_back(conn);
    log_shared_ptr_info(conn, "После добавления в global_connections");
    std::cout << "⚠️ Объект добавлен в global_connections - будет жить до конца программы!\n";
    
    // conn уничтожится здесь, но объект останется в global_connections
}

// Функция, создающая утечку через статическую переменную
void create_static_leak() {
    std::cout << "\n3️⃣ === Создаем утечку через статическую переменную ===\n";
    static_connection = std::make_shared<YamuxedConnection>();
    log_shared_ptr_info(static_connection, "После присваивания в static_connection");
    std::cout << "⚠️ Объект сохранен в static_connection - будет жить до конца программы!\n";
}

// Функция, демонстрирующая правильное использование
void create_proper_usage() {
    std::cout << "\n1️⃣ === Правильное использование ===\n";
    auto conn = std::make_shared<YamuxedConnection>();
    log_shared_ptr_info(conn, "После создания");
    
    {
        auto conn_copy = conn;
        log_shared_ptr_info(conn, "После создания копии");
        log_shared_ptr_info(conn_copy, "Копия");
        std::cout << "📋 Создана копия в локальном scope\n";
        // conn_copy уничтожится здесь
    }
    
    log_shared_ptr_info(conn, "После уничтожения копии");
    std::cout << "✅ Объект будет уничтожен при выходе из функции\n";
    // conn уничтожится здесь, и объект будет полностью освобожден
}

// Функция для показа состояния в любой момент времени
void show_current_state() {
    std::cout << "\n📊 === ТЕКУЩЕЕ СОСТОЯНИЕ ===\n";
    std::cout << "Живых объектов YamuxedConnection: " << tracked_objects.size() << "\n";
    for (const auto& pair : tracked_objects) {
        std::cout << "  - " << pair.second << " at " << pair.first << "\n";
    }
    
    std::cout << "global_connections.size(): " << global_connections.size() << "\n";
    std::cout << "static_connection.use_count(): " << (static_connection ? static_connection.use_count() : 0) << "\n";
}

int main() {
    std::cout << "🧪 === ДЕМОНСТРАЦИЯ ПРОБЛЕМЫ SHARED_PTR УТЕЧЕК ===\n";
    
    create_proper_usage();
    show_current_state();
    
    create_global_leak();
    show_current_state();
    
    create_static_leak();
    show_current_state();
    
    std::cout << "\n🎯 === ФИНАЛЬНЫЙ АНАЛИЗ ===\n";
    std::cout << "На данный момент в памяти остаются:\n";
    std::cout << "1. YamuxedConnection в global_connections[0]\n";
    std::cout << "2. YamuxedConnection в static_connection\n";
    std::cout << "\nЭти объекты будут освобождены только при завершении программы,\n";
    std::cout << "что делает их похожими на утечки памяти!\n";
    
    std::cout << "\n💡 === КАК НАЙТИ ПРОБЛЕМУ ===\n";
    std::cout << "В РЕАЛЬНОМ коде нужно отследить:\n";
    std::cout << "- Где создается shared_ptr<YamuxedConnection>\n";
    std::cout << "- Кто держит копии shared_ptr\n";
    std::cout << "- Какие контейнеры или классы не освобождают ссылки\n";
    std::cout << "- Есть ли циклические ссылки\n";
    
    return 0;
} 
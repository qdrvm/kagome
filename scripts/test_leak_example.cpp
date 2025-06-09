#include <iostream>
#include <memory>
#include <vector>

namespace libp2p::connection {
    class YamuxedConnection { 
    public: 
        char data[400]; 
        int id;
        
        YamuxedConnection(int _id) : id(_id) {
            std::cout << "🔧 YamuxedConnection #" << id << " created" << std::endl;
        }
        
        ~YamuxedConnection() {
            std::cout << "💀 YamuxedConnection #" << id << " destroyed" << std::endl;
        }
    };
}

// Глобальная переменная, которая будет держать shared_ptr и создавать утечку
std::vector<std::shared_ptr<libp2p::connection::YamuxedConnection>> global_connections;

void create_connection_in_function(int id) {
    std::cout << "📍 Creating connection in function..." << std::endl;
    auto conn = std::make_shared<libp2p::connection::YamuxedConnection>(id);
    
    // Добавляем в глобальный вектор - это создаст утечку!
    global_connections.push_back(conn);
    std::cout << "⚠️  Added to global_connections (potential leak source)" << std::endl;
}

void create_and_release_connection(int id) {
    std::cout << "📍 Creating temporary connection..." << std::endl;
    auto conn = std::make_shared<libp2p::connection::YamuxedConnection>(id);
    // Этот объект должен быть освобожден при выходе из функции
}

int main() {
    std::cout << "=== 🧪 YamuxedConnection Leak Test ===" << std::endl;
    
    // Создаем объект который будет корректно освобожден
    std::cout << "\n1. Creating normal connection (will be freed):" << std::endl;
    create_and_release_connection(1);
    
    // Создаем объекты которые будут утекать
    std::cout << "\n2. Creating leaked connections:" << std::endl;
    create_connection_in_function(2);
    create_connection_in_function(3);
    
    std::cout << "\n📊 Summary:" << std::endl;
    std::cout << "- Connection #1: properly freed" << std::endl;
    std::cout << "- Connections #2, #3: stored in global_connections (LEAKED!)" << std::endl;
    std::cout << "- Stack trace will show WHERE the leaked objects were created" << std::endl;
    
    // НЕ очищаем global_connections - это создаст утечку!
    // global_connections.clear();
    
    return 0;
} 
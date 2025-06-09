# Makefile для точного отслеживания памяти YamuxedConnection

CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2 -g -Wall -Wextra
LDFLAGS = -shared -ldl -lpthread

# Библиотеки
LIBRARY_PRECISE = libyamux_logger_precise.so
LIBRARY_HUNTER = libyamux_shared_ptr_hunter.so

# Цели
all: $(LIBRARY_PRECISE) $(LIBRARY_HUNTER)

$(LIBRARY_PRECISE): yamux_logger_precise.cpp
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $< $(LDFLAGS)

$(LIBRARY_HUNTER): yamux_shared_ptr_hunter.cpp
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $< $(LDFLAGS)

# Установка и использование
install: $(LIBRARY_PRECISE) $(LIBRARY_HUNTER)
	@echo "✅ Обе библиотеки готовы к использованию!"
	@echo ""
	@echo "🔍 БАЗОВОЕ ОТСЛЕЖИВАНИЕ (показывает где создался объект):"
	@echo "  LD_PRELOAD=./$(LIBRARY_PRECISE) ./kagome_node"
	@echo ""
	@echo "🎯 ПРОДВИНУТОЕ ОТСЛЕЖИВАНИЕ (показывает где остались shared_ptr):"
	@echo "  LD_PRELOAD=./$(LIBRARY_HUNTER) ./kagome_node"
	@echo "  (требует добавления макросов в код kagome)"
	@echo ""
	@echo "📊 МОНИТОРИНГ В РЕАЛЬНОМ ВРЕМЕНИ:"
	@echo "  tail -f yamux_shared_ptr_hunt.log"

# Тест точной библиотеки
test: $(LIBRARY_PRECISE)
	@echo "=== 🎯 Тестирование точной LD_PRELOAD библиотеки ==="
	@echo "Создаем программу с YamuxedConnection и другими yamux типами..."
	@echo '#include <iostream>' > test_yamux_precise.cpp
	@echo '#include <memory>' >> test_yamux_precise.cpp
	@echo 'namespace libp2p::connection {' >> test_yamux_precise.cpp
	@echo '  class YamuxedConnection { public: char data[400]; };' >> test_yamux_precise.cpp
	@echo '  class YamuxStream { public: char data[400]; };  // Другой yamux тип' >> test_yamux_precise.cpp
	@echo '}' >> test_yamux_precise.cpp
	@echo 'int main() {' >> test_yamux_precise.cpp
	@echo '  // Создаем YamuxedConnection (должно отслеживаться)' >> test_yamux_precise.cpp
	@echo '  auto conn = std::make_shared<libp2p::connection::YamuxedConnection>();' >> test_yamux_precise.cpp
	@echo '  // Создаем YamuxStream (НЕ должно отслеживаться)' >> test_yamux_precise.cpp
	@echo '  auto stream = std::make_shared<libp2p::connection::YamuxStream>();' >> test_yamux_precise.cpp
	@echo '  std::cout << "✅ Created YamuxedConnection and YamuxStream" << std::endl;' >> test_yamux_precise.cpp
	@echo '  return 0;' >> test_yamux_precise.cpp
	@echo '}' >> test_yamux_precise.cpp
	$(CXX) -std=c++17 -o test_yamux_precise test_yamux_precise.cpp
	@echo "Запускаем с точным трекингом..."
	LD_PRELOAD=./$(LIBRARY_PRECISE) ./test_yamux_precise
	@echo ""
	@echo "📋 РЕЗУЛЬТАТ:"
	@test -f yamux_precise.log && echo "✅ Точный лог создан" || echo "❌ Точный лог не создан"
	@if [ -f yamux_precise.log ]; then echo "=== СОДЕРЖИМОЕ ЛОГА ==="; cat yamux_precise.log; fi
	@echo ""
	@echo "🎯 Проверяем фильтрацию: должен быть только YamuxedConnection, НЕ YamuxStream!"

# Тест shared_ptr hunter библиотеки
test-hunter: $(LIBRARY_HUNTER)
	@echo "=== 🎯 Тестирование shared_ptr Hunter библиотеки ==="
	@echo "Создаем программу с shared_ptr отслеживанием..."
	@echo '#include <iostream>' > test_shared_ptr_hunter.cpp
	@echo '#include <memory>' >> test_shared_ptr_hunter.cpp
	@echo '#include <vector>' >> test_shared_ptr_hunter.cpp
	@echo '#define YAMUX_SHARED_PTR_TRACKING' >> test_shared_ptr_hunter.cpp
	@echo '#include "YAMUX_SHARED_PTR_MACROS.h"' >> test_shared_ptr_hunter.cpp
	@echo 'namespace libp2p::connection {' >> test_shared_ptr_hunter.cpp
	@echo '  class YamuxedConnection { public: char data[400]; };' >> test_shared_ptr_hunter.cpp
	@echo '}' >> test_shared_ptr_hunter.cpp
	@echo 'std::vector<std::shared_ptr<libp2p::connection::YamuxedConnection>> global_connections;' >> test_shared_ptr_hunter.cpp
	@echo 'int main() {' >> test_shared_ptr_hunter.cpp
	@echo '  std::cout << "🧪 Testing shared_ptr tracking..." << std::endl;' >> test_shared_ptr_hunter.cpp
	@echo '  // Создаем и отслеживаем shared_ptr' >> test_shared_ptr_hunter.cpp
	@echo '  auto conn1 = std::make_shared<libp2p::connection::YamuxedConnection>();' >> test_shared_ptr_hunter.cpp
	@echo '  YAMUX_TRACK_SHARED_PTR_CREATE(conn1, conn1.get());' >> test_shared_ptr_hunter.cpp
	@echo '  // Копируем в глобальный контейнер (УТЕЧКА!)' >> test_shared_ptr_hunter.cpp
	@echo '  global_connections.push_back(conn1);' >> test_shared_ptr_hunter.cpp
	@echo '  YAMUX_TRACK_SHARED_PTR_COPY(global_connections.back(), conn1);' >> test_shared_ptr_hunter.cpp
	@echo '  std::cout << "✅ Created connection and stored in global container" << std::endl;' >> test_shared_ptr_hunter.cpp
	@echo '  return 0;' >> test_shared_ptr_hunter.cpp
	@echo '}' >> test_shared_ptr_hunter.cpp
	$(CXX) -std=c++17 -I. -o test_shared_ptr_hunter test_shared_ptr_hunter.cpp
	@echo "Запускаем с shared_ptr hunter..."
	LD_PRELOAD=./$(LIBRARY_HUNTER) ./test_shared_ptr_hunter
	@echo ""
	@echo "📋 РЕЗУЛЬТАТ:"
	@test -f yamux_shared_ptr_hunt.log && echo "✅ Лог shared_ptr hunter создан" || echo "❌ Лог не создан"
	@if [ -f yamux_shared_ptr_hunt.log ]; then echo "=== СОДЕРЖИМОЕ HUNTER ЛОГА ==="; cat yamux_shared_ptr_hunt.log; fi

# Демонстрация проблемы shared_ptr
demo: simple_shared_ptr_demo.cpp
	$(CXX) -std=c++17 -g -o simple_shared_ptr_demo simple_shared_ptr_demo.cpp
	@echo "🧪 Запускаем демонстрацию проблемы shared_ptr утечек:"
	./simple_shared_ptr_demo

# Тест с утечками
test-leaks: test_leak_example.cpp $(LIBRARY_PRECISE)
	$(CXX) -std=c++17 -g -o test_leak_example test_leak_example.cpp
	@echo "🧪 Тестируем диагностику утечек shared_ptr:"
	LD_PRELOAD=./$(LIBRARY_PRECISE) ./test_leak_example

# Очистка
clean:
	rm -f $(LIBRARY_PRECISE) $(LIBRARY_HUNTER) *.log test_yamux_precise test_leak_example simple_shared_ptr_demo test_yamux_precise.cpp test_shared_ptr_hunter test_shared_ptr_hunter.cpp

.PHONY: all install test test-hunter demo test-leaks clean 
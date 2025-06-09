# 🎯 Руководство по интеграции shared_ptr Hunter в Kagome

## Проблема
Вы знаете где создается `YamuxedConnection`, но нужно найти **где остаются активные shared_ptr** которые не освобождаются.

## Решение

### 1. 🔧 Интеграция в код Kagome

#### Шаг 1: Добавить заголовок
Скопируйте `YAMUX_SHARED_PTR_MACROS.h` в папку `include/` вашего проекта.

#### Шаг 2: Включить в нужные файлы
```cpp
// В файлах где работают с shared_ptr<YamuxedConnection>
#ifdef YAMUX_DEBUG_LEAKS
#define YAMUX_SHARED_PTR_TRACKING
#include "YAMUX_SHARED_PTR_MACROS.h"
#endif
```

#### Шаг 3: Добавить макросы в ключевые места

**В местах создания shared_ptr:**
```cpp
// Было:
auto connection = std::make_shared<YamuxedConnection>(...);

// Стало:
auto connection = std::make_shared<YamuxedConnection>(...);
YAMUX_TRACK_SHARED_PTR_CREATE(connection, connection.get());
```

**В местах копирования в контейнеры:**
```cpp
// Было:
connections_.push_back(connection);

// Стало:
connections_.push_back(connection);
YAMUX_TRACK_SHARED_PTR_COPY(connections_.back(), connection);
```

**В функциях с shared_ptr параметрами:**
```cpp
void setConnection(std::shared_ptr<YamuxedConnection> conn) {
    stored_connection_ = conn;
    YAMUX_TRACK_SHARED_PTR_COPY(stored_connection_, conn);
}
```

### 2. 🚀 Использование в Production

#### Компиляция с отслеживанием:
```bash
# Сборка с флагом отслеживания
cd /path/to/kagome
cmake -DCMAKE_CXX_FLAGS="-DYAMUX_DEBUG_LEAKS" ..
make
```

#### Запуск с shared_ptr hunter:
```bash
cd /path/to/kagome/scripts
LD_PRELOAD=./libyamux_shared_ptr_hunter.so ./kagome_node
```

### 3. 📊 Анализ результатов

#### Мониторинг в реальном времени:
```bash
tail -f yamux_shared_ptr_hunt.log
```

#### Анализ утечек:
```bash
grep -A 10 "WHO IS HOLDING SHARED_PTR" yamux_shared_ptr_hunt.log
```

### 4. 🎯 Пример вывода

При запуске `LD_PRELOAD=./libyamux_shared_ptr_hunter.so ./kagome_node` вы получите:

**В файле `yamux_shared_ptr_hunt.log`:**
```
=== YamuxedConnection shared_ptr Hunter Started ===
PID: 123456
Mission: Find WHO holds shared_ptr<YamuxedConnection>
Strategy: Track shared_ptr creation/copy/destruction

🏗️ YAMUXEDCONNECTION_CREATED: 0x7f8a1c008d40 (size=416)
Created at:
    ./kagome_node(_ZN6libp2p10connection17YamuxedConnection6createEv+0x45)

🆕 SHARED_PTR_CREATED #1:
shared_ptr: 0x7ffee1234567 -> YamuxedConnection: 0x7f8a1c008d40
Created at:
    ./kagome_node(_ZN6libp2p7network12RouterImpl12createStreamEv+0x123)
    ./kagome_node(main+0x89)

📋 SHARED_PTR_COPIED:
New: 0x7ffee1234888 (copied from 0x7ffee1234567)
Points to YamuxedConnection: 0x7f8a1c008d40
Copied at:
    ./kagome_node(_ZN6libp2p7network15ConnectionPool8addConnEv+0x67)
    ./kagome_node(_ZN6libp2p7network15ConnectionPool8storeConnEv+0x23)

🎯 === WHO IS HOLDING SHARED_PTR<YAMUXEDCONNECTION>? ===
🚨 Found 1 active shared_ptr holders:

--- SHARED_PTR HOLDER #1 ---
shared_ptr address: 0x7ffee1234888
Points to YamuxedConnection: 0x7f8a1c008d40
📍 CREATED HERE:
    ./kagome_node(_ZN6libp2p7network12RouterImpl12createStreamEv+0x123)
    ./kagome_node(main+0x89)
📍 LAST COPIED HERE:
    ./kagome_node(_ZN6libp2p7network15ConnectionPool8addConnEv+0x67)
    ./kagome_node(_ZN6libp2p7network15ConnectionPool8storeConnEv+0x23)
💡 CHECK THIS CODE LOCATION! Someone holds shared_ptr here.
```

### 5. 🕵️ Интерпретация результатов

**Если видите "LAST COPIED HERE"** - это показывает ГДЕ кто-то скопировал shared_ptr и не освободил его!

**Ключевые места для проверки:**
1. **ConnectionPool** - проверьте очистку контейнера connections
2. **Static/Global переменные** - проверьте статические shared_ptr
3. **Callback функции** - проверьте lambda захваты
4. **Circular references** - проверьте взаимные ссылки

### 6. 🔍 Быстрая диагностика без изменения кода

Если не хотите менять код, используйте базовую библиотеку:
```bash
LD_PRELOAD=./libyamux_logger_precise.so ./kagome_node
```

Она покажет где создаются объекты, а затем вы можете искать места их удержания вручную.

### 7. 💡 Практические советы

1. **Начните с простого:** сначала используйте `libyamux_logger_precise.so`
2. **Найдите паттерн:** если утечки растут - ищите контейнеры или static переменные
3. **Используйте GDB:** поставьте breakpoint на деструктор `~YamuxedConnection()`
4. **Проверьте callbacks:** лямбды часто захватывают shared_ptr

### 8. 🚨 Решение типичных проблем

**Утечка в ConnectionPool:**
```cpp
// Проблема: connections_ не очищается
std::vector<std::shared_ptr<YamuxedConnection>> connections_;

// Решение: добавить очистку
void cleanup() {
    connections_.clear();  // Освобождает все shared_ptr
}
```

**Утечка в callback:**
```cpp
// Проблема: lambda захватывает shared_ptr
auto callback = [connection = shared_conn](){ /* ... */ };

// Решение: используйте weak_ptr
auto callback = [weak_conn = std::weak_ptr(shared_conn)](){
    if (auto conn = weak_conn.lock()) { /* ... */ }
};
```

Теперь у вас есть полный набор инструментов для охоты на shared_ptr утечки! 🎯 
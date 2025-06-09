#define _GNU_SOURCE
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <execinfo.h>
#include <atomic>
#include <algorithm>
#include <map>
#include <string>

static std::atomic<bool> initialized{false};
static std::atomic<int> alloc_count{0};
static std::atomic<int> free_count{0};
static FILE* log_file = nullptr;
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;

// Структура для хранения информации об аллокации
struct AllocInfo {
    size_t size;
    std::string stack_trace;
    int alloc_id;
};

static std::map<void*, AllocInfo> active_allocs;
static std::atomic<int> next_alloc_id{1};

void __attribute__((constructor)) init_logger() {
    const char* log_filename = getenv("YAMUX_LOG_FILE");
    if (!log_filename) log_filename = "yamux_precise.log";
    
    log_file = fopen(log_filename, "w");
    if (log_file) {
        fprintf(log_file, "=== Enhanced YamuxedConnection Tracker Started ===\n");
        fprintf(log_file, "PID: %d\n", getpid());
        fprintf(log_file, "Features: Stack trace + shared_ptr leak detection\n");
        fprintf(log_file, "Expected YamuxedConnection size: 400-450 bytes\n");
        fflush(log_file);
        initialized = true;
    }
}

void __attribute__((destructor)) cleanup_logger() {
    if (log_file) {
        fprintf(log_file, "\n=== LEAK ANALYSIS ===\n");
        fprintf(log_file, "Total malloc calls: %d\n", alloc_count.load());
        fprintf(log_file, "Total free calls: %d\n", free_count.load());
        fprintf(log_file, "Active leaks: %zu\n", active_allocs.size());
        
        if (!active_allocs.empty()) {
            fprintf(log_file, "\n🚨 LEAKED YAMUXEDCONNECTION OBJECTS:\n");
            for (const auto& leak : active_allocs) {
                fprintf(log_file, "\n--- LEAK #%d ---\n", leak.second.alloc_id);
                fprintf(log_file, "Pointer: %p\n", leak.first);
                fprintf(log_file, "Size: %zu bytes\n", leak.second.size);
                fprintf(log_file, "Created at:\n%s\n", leak.second.stack_trace.c_str());
                fprintf(log_file, "💡 This shows WHERE the YamuxedConnection was created.\n");
                fprintf(log_file, "   Check who holds shared_ptr to this object!\n");
            }
        }
        
        fclose(log_file);
    }
}

static bool is_yamuxed_connection_size(size_t size) {
    // YamuxedConnection в тестах: 416 байт (400 данных + накладные расходы)
    // Более точная эвристика для YamuxedConnection
    return (size >= 400 && size <= 450);
}

// Получить полный stack trace
static std::string get_stack_trace() {
    void* buffer[32];
    int nptrs = backtrace(buffer, 32);
    char** strings = backtrace_symbols(buffer, nptrs);
    
    std::string result;
    if (strings != nullptr) {
        for (int i = 0; i < nptrs; i++) {
            // Пропускаем кадры самой библиотеки
            if (strstr(strings[i], "libyamux_logger") != nullptr ||
                strstr(strings[i], "malloc") != nullptr ||
                strstr(strings[i], "ld-linux") != nullptr) {
                continue;
            }
            
            result += "  ";
            result += strings[i];
            result += "\n";
            
            // Ограничиваем количество кадров для читаемости
            if (result.length() > 1000) {
                result += "  ... (truncated)\n";
                break;
            }
        }
        free(strings);
    }
    
    if (result.empty()) {
        result = "  (no useful stack trace available)\n";
    }
    
    return result;
}

extern "C" {

void* malloc(size_t size) {
    static std::atomic<bool> in_malloc{false};
    
    if (!real_malloc) {
        real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    }
    
    void* ptr = real_malloc(size);
    
    // Избегаем рекурсии при логировании
    if (initialized && log_file && !in_malloc.exchange(true)) {
        
        // Точная проверка размера для YamuxedConnection
        if (is_yamuxed_connection_size(size)) {
            int alloc_id = next_alloc_id++;
            alloc_count++;
            
            // Получаем stack trace
            std::string stack_trace = get_stack_trace();
            
            // Сохраняем информацию об аллокации
            active_allocs[ptr] = {size, stack_trace, alloc_id};
            
            fprintf(log_file, "\n📝 YAMUX_MALLOC #%d:\n", alloc_id);
            fprintf(log_file, "Pointer: %p\n", ptr);
            fprintf(log_file, "Size: %zu bytes\n", size);
            fprintf(log_file, "Stack trace:\n%s", stack_trace.c_str());
            fflush(log_file);
        }
        
        in_malloc = false;
    }
    
    return ptr;
}

void free(void* ptr) {
    if (!ptr) return;
    
    static std::atomic<bool> in_free{false};
    
    if (!real_free) {
        real_free = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    }
    
    if (initialized && log_file && !in_free.exchange(true)) {
        
        // Проверяем, был ли это отслеживаемый объект
        auto it = active_allocs.find(ptr);
        if (it != active_allocs.end()) {
            free_count++;
            fprintf(log_file, "\n✅ YAMUX_FREE #%d: ptr=%p (size=%zu)\n", 
                   it->second.alloc_id, ptr, it->second.size);
            fflush(log_file);
            
            // Удаляем из списка активных аллокаций
            active_allocs.erase(it);
        }
        
        in_free = false;
    }
    
    real_free(ptr);
}

} // extern "C" 
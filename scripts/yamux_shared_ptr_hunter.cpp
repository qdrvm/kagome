#define _GNU_SOURCE
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <execinfo.h>
#include <atomic>
#include <map>
#include <string>
#include <mutex>

static std::atomic<bool> initialized{false};
static FILE* log_file = nullptr;
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;

// Структура для хранения информации о shared_ptr держателях
struct SharedPtrInfo {
    void* yamux_object;           // Указатель на YamuxedConnection
    std::string creation_stack;   // Где был создан shared_ptr
    std::string last_copy_stack;  // Где последний раз копировался
    int ref_count_hint;           // Примерная оценка числа ссылок
    bool is_leaked;               // Помечен как утечка
};

static std::map<void*, SharedPtrInfo> active_shared_ptrs;
static std::map<void*, std::string> yamux_objects; // yamux ptr -> creation stack
static std::mutex tracker_mutex;
static std::atomic<int> next_shared_ptr_id{1};

void __attribute__((constructor)) init_shared_ptr_hunter() {
    const char* log_filename = getenv("YAMUX_LOG_FILE");
    if (!log_filename) log_filename = "yamux_shared_ptr_hunt.log";
    
    log_file = fopen(log_filename, "w");
    if (log_file) {
        fprintf(log_file, "=== YamuxedConnection shared_ptr Hunter Started ===\n");
        fprintf(log_file, "PID: %d\n", getpid());
        fprintf(log_file, "Mission: Find WHO holds shared_ptr<YamuxedConnection>\n");
        fprintf(log_file, "Strategy: Track shared_ptr creation/copy/destruction\n\n");
        fflush(log_file);
        initialized = true;
    }
}

void __attribute__((destructor)) cleanup_shared_ptr_hunter() {
    if (log_file) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        
        fprintf(log_file, "\n🎯 === WHO IS HOLDING SHARED_PTR<YAMUXEDCONNECTION>? ===\n");
        
        if (active_shared_ptrs.empty()) {
            fprintf(log_file, "✅ No active shared_ptr holders found - no leaks!\n");
        } else {
            fprintf(log_file, "🚨 Found %zu active shared_ptr holders:\n\n", active_shared_ptrs.size());
            
            int leak_id = 1;
            for (const auto& holder : active_shared_ptrs) {
                fprintf(log_file, "--- SHARED_PTR HOLDER #%d ---\n", leak_id++);
                fprintf(log_file, "shared_ptr address: %p\n", holder.first);
                fprintf(log_file, "Points to YamuxedConnection: %p\n", holder.second.yamux_object);
                fprintf(log_file, "📍 CREATED HERE:\n%s\n", holder.second.creation_stack.c_str());
                if (!holder.second.last_copy_stack.empty()) {
                    fprintf(log_file, "📍 LAST COPIED HERE:\n%s\n", holder.second.last_copy_stack.c_str());
                }
                fprintf(log_file, "💡 CHECK THIS CODE LOCATION! Someone holds shared_ptr here.\n\n");
            }
        }
        
        fclose(log_file);
    }
}

static bool is_yamuxed_connection_size(size_t size) {
    return (size >= 400 && size <= 450);
}

static std::string get_stack_trace(const char* context) {
    void* buffer[16];
    int nptrs = backtrace(buffer, 16);
    char** strings = backtrace_symbols(buffer, nptrs);
    
    std::string result;
    if (strings != nullptr) {
        for (int i = 1; i < nptrs; i++) { // Пропускаем сам get_stack_trace
            // Пропускаем внутренние функции библиотеки
            if (strstr(strings[i], "yamux_shared_ptr_hunter") != nullptr ||
                strstr(strings[i], "malloc") != nullptr ||
                strstr(strings[i], "ld-linux") != nullptr ||
                strstr(strings[i], "_dl_") != nullptr) {
                continue;
            }
            
            result += "    ";
            result += strings[i];
            result += "\n";
            
            // Показываем только самые важные кадры
            if (result.length() > 800) {
                result += "    ... (more frames)\n";
                break;
            }
        }
        free(strings);
    }
    
    if (result.empty()) {
        result = "    (stack trace unavailable)\n";
    }
    
    return result;
}

// Функции для внешнего вызова из кода (через макросы)
extern "C" {

// Вызывается когда создается shared_ptr<YamuxedConnection>
void __yamux_shared_ptr_created(void* shared_ptr_addr, void* yamux_object) {
    if (!initialized || !log_file) return;
    
    std::lock_guard<std::mutex> lock(tracker_mutex);
    
    int sp_id = next_shared_ptr_id++;
    std::string stack = get_stack_trace("shared_ptr creation");
    
    active_shared_ptrs[shared_ptr_addr] = {
        yamux_object,
        stack,
        "",  // no copy yet
        1,   // initial ref count
        false
    };
    
    fprintf(log_file, "🆕 SHARED_PTR_CREATED #%d:\n", sp_id);
    fprintf(log_file, "shared_ptr: %p -> YamuxedConnection: %p\n", shared_ptr_addr, yamux_object);
    fprintf(log_file, "Created at:\n%s\n", stack.c_str());
    fflush(log_file);
}

// Вызывается когда копируется shared_ptr<YamuxedConnection>
void __yamux_shared_ptr_copied(void* new_shared_ptr_addr, void* old_shared_ptr_addr) {
    if (!initialized || !log_file) return;
    
    std::lock_guard<std::mutex> lock(tracker_mutex);
    
    // Находим исходный shared_ptr
    auto old_it = active_shared_ptrs.find(old_shared_ptr_addr);
    if (old_it != active_shared_ptrs.end()) {
        std::string stack = get_stack_trace("shared_ptr copy");
        
        // Создаем новую запись для копии
        active_shared_ptrs[new_shared_ptr_addr] = {
            old_it->second.yamux_object,  // тот же yamux объект
            old_it->second.creation_stack, // исходный creation stack
            stack,  // где скопировали
            old_it->second.ref_count_hint + 1,
            false
        };
        
        fprintf(log_file, "📋 SHARED_PTR_COPIED:\n");
        fprintf(log_file, "New: %p (copied from %p)\n", new_shared_ptr_addr, old_shared_ptr_addr);
        fprintf(log_file, "Points to YamuxedConnection: %p\n", old_it->second.yamux_object);
        fprintf(log_file, "Copied at:\n%s\n", stack.c_str());
        fflush(log_file);
    }
}

// Вызывается когда уничтожается shared_ptr<YamuxedConnection>
void __yamux_shared_ptr_destroyed(void* shared_ptr_addr) {
    if (!initialized || !log_file) return;
    
    std::lock_guard<std::mutex> lock(tracker_mutex);
    
    auto it = active_shared_ptrs.find(shared_ptr_addr);
    if (it != active_shared_ptrs.end()) {
        fprintf(log_file, "💀 SHARED_PTR_DESTROYED: %p (was pointing to %p)\n", 
               shared_ptr_addr, it->second.yamux_object);
        fflush(log_file);
        
        active_shared_ptrs.erase(it);
    }
}

// Стандартные malloc/free для отслеживания YamuxedConnection объектов
void* malloc(size_t size) {
    static std::atomic<bool> in_malloc{false};
    
    if (!real_malloc) {
        real_malloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    }
    
    void* ptr = real_malloc(size);
    
    if (initialized && log_file && !in_malloc.exchange(true)) {
        if (is_yamuxed_connection_size(size)) {
            std::lock_guard<std::mutex> lock(tracker_mutex);
            
            std::string stack = get_stack_trace("YamuxedConnection creation");
            yamux_objects[ptr] = stack;
            
            fprintf(log_file, "🏗️  YAMUXEDCONNECTION_CREATED: %p (size=%zu)\n", ptr, size);
            fprintf(log_file, "Created at:\n%s\n", stack.c_str());
            fflush(log_file);
        }
        in_malloc = false;
    }
    
    return ptr;
}

void free(void* ptr) {
    if (!ptr) return;
    
    if (!real_free) {
        real_free = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    }
    
    if (initialized && log_file) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        
        auto it = yamux_objects.find(ptr);
        if (it != yamux_objects.end()) {
            fprintf(log_file, "🗑️  YAMUXEDCONNECTION_DESTROYED: %p\n", ptr);
            fflush(log_file);
            yamux_objects.erase(it);
        }
    }
    
    real_free(ptr);
}

} // extern "C" 
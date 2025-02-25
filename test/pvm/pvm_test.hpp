
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <functional>
#include <string>
#include <cstring>
#include <immintrin.h> // SIMD-инструкции
#include <thread>
#include <mutex>
#include <atomic>

class RiscVVM {
public:
    RiscVVM() : pc(0), memory(1024 * 1024, 0) { // 1 МБ памяти
        registers.resize(32, 0);
        registers[0] = 0; // x0 всегда равен нулю


        // Инициализация CSR
        csr["mstatus"] = 0;
        csr["mcause"] = 0;
        csr["mepc"] = 0;
        csr["mtvec"] = 0x1000; // Вектор исключений по умолчанию
    }

    void loadProgram(const std::vector<uint64_t>& program) {
        for (size_t i = 0; i < program.size(); ++i) {
            memory[i] = program[i];
        }
    }

    void run() {
        std::vector<std::thread> threads;
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {

            threads.emplace_back(&RiscVVM::executeThread, this);
        }
        for (auto& t : threads) {
            t.join();
        }
    }

    void printRegisters() const {
        std::lock_guard<std::mutex> lock(printMutex);
        for (size_t i = 0; i < registers.size(); ++i) {
            std::cout << "x" << i << ": " << registers[i] << std::endl;
        }
    }

    // Регистрация внешней функции
    void registerExternalFunction(uint64_t id, std::function<int64_t(int64_t*, uint64_t)> func) {
        externalFunctions[id] = func;
    }

private:
    std::vector<int64_t> registers; // 64-битные регистры x0-x31
    std::vector<uint64_t> memory;   // Физическая память
    std::atomic<uint64_t> pc;       // Атомарный счетчик команд
    mutable std::mutex memoryMutex;
    mutable std::mutex printMutex;

    // Таблица страниц для виртуальной памяти
    std::unordered_map<uint64_t, uint64_t> pageTable;

    // Регистры состояния (CSR)

    std::unordered_map<std::string, uint64_t> csr;

    // Таблица внешних функций
    std::unordered_map<uint64_t, std::function<int64_t(int64_t*, uint64_t)>> externalFunctions;

    // Проверка выравнивания адреса
    bool isAddressAligned(uint64_t addr, uint64_t alignment) {
        return (addr % alignment) == 0;
    }

    // Преобразование виртуального адреса в физический
    uint64_t translateAddress(uint64_t vaddr) {
        uint64_t pageNumber = vaddr / 4096; // Номер страницы
        uint64_t offset = vaddr % 4096;     // Смещение в странице

        if (pageTable.find(pageNumber) == pageTable.end()) {
            throw std::runtime_error("Page fault: page not found");
        }

        return pageTable[pageNumber] * 4096 + offset;
    }

    // Чтение из памяти
    uint64_t readMemory(uint64_t addr) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        uint64_t paddr = translateAddress(addr);
        return memory[paddr / 8];
    }

    // Запись в память
    void writeMemory(uint64_t addr, uint64_t value) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        uint64_t paddr = translateAddress(addr);
        memory[paddr / 8] = value;
    }

    // Чтение строки из памяти
    std::string readString(uint64_t addr) {
        std::string result;
        while (true) {
            uint64_t paddr = translateAddress(addr);
            char c = static_cast<char>(memory[paddr / 8] & 0xFF);
            if (c == '\0') break;
            result += c;

            addr++;
        }
        return result;
    }

    // Запись строки в память
    void writeString(uint64_t addr, const std::string& str) {
        for (size_t i = 0; i < str.size(); ++i) {
            uint64_t paddr = translateAddress(addr + i);
            memory[paddr / 8] = static_cast<uint64_t>(str[i]) & 0xFF;
        }
        // Записываем завершающий нулевой символ
        uint64_t paddr = translateAddress(addr + str.size());
        memory[paddr / 8] = 0;
    }

    // Чтение массива из памяти
    std::vector<int64_t> readArray(uint64_t addr, uint64_t size) {
        std::vector<int64_t> result(size);
        for (uint64_t i = 0; i < size; ++i) {
            uint64_t paddr = translateAddress(addr + i * sizeof(int64_t));
            result[i] = static_cast<int64_t>(memory[paddr / 8]);
        }
        return result;
    }

    // Запись массива в память
    void writeArray(uint64_t addr, const std::vector<int64_t>& array) {
        for (uint64_t i = 0; i < array.size(); ++i) {
            uint64_t paddr = translateAddress(addr + i * sizeof(int64_t));
            memory[paddr / 8] = static_cast<uint64_t>(array[i]);
        }
    }

    // Чтение структуры из памяти
    template <typename T>
    T readStruct(uint64_t addr) {
        T result;
        uint64_t size = sizeof(T);
        uint64_t paddr = translateAddress(addr);
        std::memcpy(&result, &memory[paddr / 8], size);
        return result;
    }

    // Запись структуры в память
    template <typename T>
    void writeStruct(uint64_t addr, const T& value) {
        uint64_t size = sizeof(T);
        uint64_t paddr = translateAddress(addr);
        std::memcpy(&memory[paddr / 8], &value, size);
    }

    // Обработка исключений
    void handleException(const std::string& cause, uint64_t faultAddr = 0) {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cerr << "Exception: " << cause << " at address 0x" << std::hex << faultAddr << std::dec << std::endl;

        // Сохраняем причину исключения
        if (cause == "Page fault") {
            csr["mcause"] = 12; // Page Fault
        } else if (cause == "Illegal instruction") {
            csr["mcause"] = 2; // Illegal Instruction
        } else if (cause == "Load address misaligned") {
            csr["mcause"] = 4; // Load Address Misaligned
        } else if (cause == "Store address misaligned") {
            csr["mcause"] = 6; // Store Address Misaligned
        }

        // Сохраняем адрес возврата
        csr["mepc"] = pc;

        // Переход к вектору исключений
        pc = csr["mtvec"];
    }

    // Вызов внешней функции
    void callExternalFunction(uint64_t id) {
        if (externalFunctions.find(id) == externalFunctions.end()) {
            throw std::runtime_error("External function not found");
        }

        // Передача аргументов через регистры a0-a7
        int64_t args[8] = {registers[10], registers[11], registers[12], registers[13],
                           registers[14], registers[15], registers[16], registers[17]};

        // Передача дополнительных данных через память (например, структур)
        uint64_t memArg = registers[18]; // a8 содержит адрес в памяти для дополнительных данных

        // Вызов внешней функции
        int64_t result = externalFunctions[id](args, memArg);

        // Возврат результата через регистр a0
        registers[10] = result;
    }

    // Декодирование инструкции
    void executeInstruction(uint64_t instruction) {
        uint64_t opcode = instruction & 0x7F;
        uint64_t rd = (instruction >> 7) & 0x1F;
        uint64_t funct3 = (instruction >> 12) & 0x7;
        uint64_t rs1 = (instruction >> 15) & 0x1F;
        uint64_t rs2 = (instruction >> 20) & 0x1F;
        uint64_t funct7 = (instruction >> 25) & 0x7F;

        int64_t imm_i = (instruction >> 20) & 0xFFF;

        int64_t imm_s = ((instruction >> 25) << 5) | ((instruction >> 7) & 0x1F);
        int64_t imm_b = ((instruction >> 31) << 12) | ((instruction >> 7) & 0x1E) | ((instruction >> 25) << 5) | ((instruction >> 8) << 11);
        int64_t imm_u = instruction & 0xFFFFF000;
        int64_t imm_j = ((instruction >> 31) << 20) | ((instruction >> 21) << 1) | ((instruction >> 20) << 11) | ((instruction >> 12) << 12);

        // Знаковое расширение
        if (imm_i & 0x800) imm_i |= 0xFFFFFFFFFFFFF000;
        if (imm_s & 0x800) imm_s |= 0xFFFFFFFFFFFFF000;
        if (imm_b & 0x1000) imm_b |= 0xFFFFFFFFFFFFE000;
        if (imm_j & 0x100000) imm_j |= 0xFFFFFFFFFFE00000;

        switch (opcode) {
            // R-тип (64-битные операции)
            case 0x33:
                switch (funct3) {
                    case 0x0: // ADD или SUB
                        if (funct7 == 0x00) registers[rd] = registers[rs1] + registers[rs2]; // ADD
                        else if (funct7 == 0x20) registers[rd] = registers[rs1] - registers[rs2]; // SUB
                        break;
                    case 0x1: registers[rd] = registers[rs1] << (registers[rs2] & 0x3F); break; // SLL
                    case 0x2: registers[rd] = (registers[rs1] < registers[rs2]) ? 1 : 0; break; // SLT
                    case 0x3: registers[rd] = (static_cast<uint64_t>(registers[rs1]) < static_cast<uint64_t>(registers[rs2])) ? 1 : 0; break; // SLTU
                    case 0x4: registers[rd] = registers[rs1] ^ registers[rs2]; break; // XOR
                    case 0x5: // SRL или SRA
                        if (funct7 == 0x00) registers[rd] = static_cast<uint64_t>(registers[rs1]) >> (registers[rs2] & 0x3F); // SRL
                        else if (funct7 == 0x20) registers[rd] = registers[rs1] >> (registers[rs2] & 0x3F); // SRA
                        break;
                    case 0x6: registers[rd] = registers[rs1] | registers[rs2]; break; // OR
                    case 0x7: registers[rd] = registers[rs1] & registers[rs2]; break; // AND
                }
                pc += 4;
                break;


            // I-тип (64-битные операции)
            case 0x13:
                switch (funct3) {
                    case 0x0: registers[rd] = registers[rs1] + imm_i; break; // ADDI
                    case 0x1: registers[rd] = registers[rs1] << (imm_i & 0x3F); break; // SLLI
                    case 0x2: registers[rd] = (registers[rs1] < imm_i) ? 1 : 0; break; // SLTI
                    case 0x3: registers[rd] = (static_cast<uint64_t>(registers[rs1]) < static_cast<uint64_t>(imm_i)) ? 1 : 0; break; // SLTIU
                    case 0x4: registers[rd] = registers[rs1] ^ imm_i; break; // XORI
                    case 0x5: // SRLI или SRAI
                        if (funct7 == 0x00) registers[rd] = static_cast<uint64_t>(registers[rs1]) >> (imm_i & 0x3F); // SRLI
                        else if (funct7 == 0x20) registers[rd] = registers[rs1] >> (imm_i & 0x3F); // SRAI
                        break;
                    case 0x6: registers[rd] = registers[rs1] | imm_i; break; // ORI
                    case 0x7: registers[rd] = registers[rs1] & imm_i; break; // ANDI
                }
                pc += 4;
                break;

            // S-тип (64-битные операции)
            case 0x23:
                switch (funct3) {
                    case 0x0: // SB
                        if (!isAddressAligned(registers[rs1] + imm_s, 1)) {
                            handleException("Store address misaligned", registers[rs1] + imm_s);
                            break;
                        }
                        writeMemory(registers[rs1] + imm_s, registers[rs2] & 0xFF);
                        break;
                    case 0x1: // SH
                        if (!isAddressAligned(registers[rs1] + imm_s, 2)) {
                            handleException("Store address misaligned", registers[rs1] + imm_s);
                            break;
                        }
                        writeMemory(registers[rs1] + imm_s, registers[rs2] & 0xFFFF);
                        break;
                    case 0x2: // SW
                        if (!isAddressAligned(registers[rs1] + imm_s, 4)) {
                            handleException("Store address misaligned", registers[rs1] + imm_s);
                            break;
                        }
                        writeMemory(registers[rs1] + imm_s, registers[rs2] & 0xFFFFFFFF);
                        break;
                    case 0x3: // SD
                        if (!isAddressAligned(registers[rs1] + imm_s, 8)) {
                            handleException("Store address misaligned", registers[rs1] + imm_s);
                            break;
                        }
                        writeMemory(registers[rs1] + imm_s, registers[rs2]);

                        break;
                }
                pc += 4;
                break;

            // B-тип (условные переходы)
            case 0x63:
                switch (funct3) {
                    case 0x0: if (registers[rs1] == registers[rs2]) pc += imm_b; else pc += 4; break; // BEQ
                    case 0x1: if (registers[rs1] != registers[rs2]) pc += imm_b; else pc += 4; break; // BNE
                    case 0x4: if (registers[rs1] < registers[rs2]) pc += imm_b; else pc += 4; break; // BLT
                    case 0x5: if (registers[rs1] >= registers[rs2]) pc += imm_b; else pc += 4; break; // BGE
                    case 0x6: if (static_cast<uint64_t>(registers[rs1]) < static_cast<uint64_t>(registers[rs2])) pc += imm_b; else pc += 4; break; // BLTU
                    case 0x7: if (static_cast<uint64_t>(registers[rs1]) >= static_cast<uint64_t>(registers[rs2])) pc += imm_b; else pc += 4; break; // BGEU
                }
                break;

            // U-тип (64-битные операции)
            case 0x37: registers[rd] = imm_u; pc += 4; break; // LUI
            case 0x17: registers[rd] = pc + imm_u; pc += 4; break; // AUIPC

            // J-тип (безусловный переход)
            case 0x6F: registers[rd] = pc + 4; pc += imm_j; break; // JAL

            // Системные вызовы (ECALL)

            case 0x73:
                if (funct3 == 0x0) {
                    callExternalFunction(registers[17]); // a7 содержит идентификатор функции
                }
                pc += 4;
                break;

            // Расширение M (умножение и деление)
            case 0x33:
                if (funct7 == 0x01) {
                    switch (funct3) {
                        case 0x0: registers[rd] = registers[rs1] * registers[rs2]; break; // MUL
                        case 0x1: registers[rd] = (static_cast<int128_t>(registers[rs1]) * static_cast<int128_t>(registers[rs2])) >> 64; break; // MULH
                        case 0x2: registers[rd] = (static_cast<uint128_t>(registers[rs1]) * static_cast<uint128_t>(registers[rs2])) >> 64; break; // MULHU
                        case 0x3: registers[rd] = (static_cast<int128_t>(registers[rs1]) * static_cast<uint128_t>(registers[rs2])) >> 64; break; // MULHSU
                        case 0x4: registers[rd] = registers[rs1] / registers[rs2]; break; // DIV
                        case 0x5: registers[rd] = static_cast<uint64_t>(registers[rs1]) / static_cast<uint64_t>(registers[rs2]); break; // DIVU
                        case 0x6: registers[rd] = registers[rs1] % registers[rs2]; break; // REM
                        case 0x7: registers[rd] = static_cast<uint64_t>(registers[rs1]) % static_cast<uint64_t>(registers[rs2]); break; // REMU
                    }

                }
                pc += 4;
                break;

            default:
                handleException("Illegal instruction");
        }
    }

    // Поток выполнения
    static void executeThread() {
        while (true) {
            uint64_t current_pc = pc.fetch_add(4);
            if (current_pc >= memory.size()) break;

            try {
                uint64_t instruction = readMemory(current_pc);
                executeInstruction(instruction);
            } catch (const std::exception& e) {
                handleException(e.what(), current_pc);
            }
        }
    }
};

// Пример структуры
struct ExampleStruct {
    int64_t field1;
    int64_t field2;
    char field3[32];
};

// Пример внешней функции для чтения структуры
int64_t readStructFunction(int64_t* args, uint64_t memArg) {
    RiscVVM* vm = 

reinterpret_cast<RiscVVM*>(args[0]); // Передаем указатель на виртуальную машину
    ExampleStruct s = vm->readStruct<ExampleStruct>(memArg); // Чтение структуры из памяти
    std::cout << "Struct fields: " << s.field1 << ", " << s.field2 << ", " << s.field3 << std::endl;
    return s.field1 + s.field2; // Возвращаем сумму полей
}

// Пример внешней функции для записи структуры
int64_t writeStructFunction(int64_t* args, uint64_t memArg) {
    RiscVVM* vm = reinterpret_cast<RiscVVM*>(args[0]); // Передаем указатель на виртуальную машину
    ExampleStruct s;

    s.field1 = args[1]; // Поле 1
    s.field2 = args[2]; // Поле 2
    std::strcpy(s.field3, "Hello, RISC-V!"); // Поле 3
    vm->writeStruct(memArg, s); // Запись структуры в память
    return sizeof(ExampleStruct); // Возвращаем размер структуры
}

int main() {
    RiscVVM vm;

    // Регистрация внешних функций
    vm.registerExternalFunction(1, readStructFunction);
    vm.registerExternalFunction(2, writeStructFunction);

    // Пример программы для RV64I с вызовом внешних функций

    std::vector<uint64_t> program = {
        0x00500093, // ADDI x1, x0, 5
        0x00A08113, // ADDI x2, x1, 10
        0x00108893, // ADDI x17, x1, 1 (идентификатор функции readStruct)
        0x00000073, // ECALL (вызов внешней функции readStruct)
        0x00208893, // ADDI x17, x1, 2 (идентификатор функции writeStruct)
        0x00000073  // ECALL (вызов внешней функции writeStruct)
    };

    // Загрузка структуры в память
    ExampleStruct testStruct = {42, 100, "Test"};
    vm.writeStruct(0x1000, testStruct); // Записываем структуру по адресу 0x1000

    // Передача адреса структуры в регистр a8

    vm.registers[18] = 0x1000;

    // Передача указателя на виртуальную машину в регистр a0
    vm.registers[10] = reinterpret_cast<int64_t>(&vm);

    // Передача значений для записи структуры в регистры a1 и a2
    vm.registers[11] = 123; // Поле 1
    vm.registers[12] = 456; // Поле 2

    vm.loadProgram(program);
    vm.run();
    vm.printRegisters();

    return 0;
}
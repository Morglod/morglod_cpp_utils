extern "C" int __cdecl sum(int a, int b) {
    std::cout << "EBAL";
    return a + b;
}

int main() {
    std::vector<int> argsBuffer = { 10, 20 };
    int* argsPtr = argsBuffer.data();

    int result;
    __asm__ volatile (
        "movl (%1), %%edi;"        // Load first argument to edi (rdi)
        "movl 4(%1), %%esi;"       // Load second argument to esi (rsi)
        "call *%2;"                // Call the function through the function pointer
        "movl %%eax, %0;"          // Store the result
        : "=r" (result)            // Output list
        : "r" (argsPtr), "r" (sum) // Input list
        : "%edi", "%esi", "%rax"   // Clobbered register list
    );

    std::cout << "result " << result << std::endl;

    return 0;
}

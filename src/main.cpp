#include <iostream>
#include <fstream>

int main() {
    {
        std::ofstream out("input.bin", std::ios_base::binary | std::ios_base::out);

        uint32_t size = 10;
        uint32_t size2 = 20;

        out.write(reinterpret_cast<char *>(&size), sizeof(size));
        out.write(reinterpret_cast<char *>(&size2), sizeof(size2));
    }

    {
        std::ifstream in("input.bin", std::ios_base::binary | std::ios_base::in);

        uint32_t size = 0;
        uint32_t size2 = 0;

        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        in.read(reinterpret_cast<char *>(&size2), sizeof(size2));

        std::cout << size << ' ' << size2 << std::endl;
    }
}

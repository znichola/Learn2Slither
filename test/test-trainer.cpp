#include "environment.hpp"
#include "interpreter.hpp"
#include "trainer.hpp"
#include "snapshot.hpp"


int ret = 0;

int main() {
    std::cout << "Testing Trainer\n";
    std::cout << "===================\n";

    train();

    return ret;
}


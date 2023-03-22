#include "lib/parser.h"
#include <string>
#include <iostream>

int main(int, char**) {
    std::string inp = "[sec1.sec2.sec3]\nkey1 = 5\n";
    omfl::OMFLParser parsed = omfl::parse(inp);

    /*std::cout << parsed.Get("key1").AsString() << '\n';*/
    std::cout << parsed.Get("sec1.sec2.sec3.key1").AsIntOrDefault(2);
    return 0;
}
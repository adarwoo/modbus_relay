#include <asx/ioport.hpp>

using namespace asx::ioport;

auto mypin = Pin{A, 5}.init(dir::out);

void other() {
   mypin.set();
}
#include "Driver/Driver.h"

int main(int argc, char** argv) {
  return fire::Driver::get_instance()->main(argc, argv);
}
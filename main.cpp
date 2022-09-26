#include "src/master.h"
#include "src/slave.h"

#define IS_MASTER true

int main() 
{
    if (IS_MASTER) {
        master_main();
    } else {
        slave_main();
    }
}
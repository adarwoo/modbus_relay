#include <logger.h>
#include <unistd.h>
#include <avr/io.h>

#include <cstddef>
#include <vector>
#include <functional>
#include <initializer_list>

extern "C" void sysclk_init() {}

extern "C" void interrupt_USART1_RXC_vect();
extern "C" void interrupt_TCA0_CMP0_vect();
extern "C" void interrupt_TCA0_CMP1_vect();
extern "C" void interrupt_TCA0_OVF_vect();
extern "C" void interrupt_USART1_TXC_vect();

// Override the logger init here
extern "C" const char *log_get_config_string()
   { return "stdout debug"; }

// Holds all the MCA registers here
char sim_registers[0x1301];

using action = std::function<void()>;
using test = std::initializer_list<action>;

test TEST_SEND_RECEIVE = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 44;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xff; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xCB; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x87; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

test TEST_WRITE_MULTIPLE_COILS = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 0x2C;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x0F; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x03; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x07; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x0D; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x14; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

test TEST_TURN_ON = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 0x2C;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x05; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xFF; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x8A; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x47; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

test TEST_IGNORE = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 45;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xff; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xCB; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x87; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

test TEST_INTERRUPTED_THEN_RESUMED = {
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ USART1.RXDATAL = 44;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); }, // Break it
    [](){ interrupt_TCA0_CMP1_vect(); }, // Make a frame (should fail!)
    [](){ USART1.RXDATAL = 44;   interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xff; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x00; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x01; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0xCB; interrupt_USART1_RXC_vect(); },
    [](){ USART1.RXDATAL = 0x87; interrupt_USART1_RXC_vect(); },
    [](){ interrupt_TCA0_CMP0_vect(); },
    [](){ interrupt_TCA0_CMP1_vect(); },
    [](){ interrupt_TCA0_OVF_vect(); /* Reply should be sent */ },
    [](){ interrupt_USART1_TXC_vect(); /* Buffer sent! */ },
    [](){ interrupt_TCA0_CMP1_vect(); /* 35 elapsed */ }
};

std::vector<test> ALL_TESTS = {
    TEST_SEND_RECEIVE,
    TEST_IGNORE,
    TEST_INTERRUPTED_THEN_RESUMED,
    TEST_TURN_ON,
    TEST_WRITE_MULTIPLE_COILS
};
auto itTest = ALL_TESTS.begin();
auto itAction = itTest->begin();

extern "C" void sleep_cpu()
{
    (*itAction++)();

    if ( itAction == itTest->end() ) {
        itTest++;
        LOG_MILE("TEST", "------------------------------------------------------------------");
        if ( itTest == ALL_TESTS.end() ) _exit(0);
        itAction = itTest->begin();
    }
}
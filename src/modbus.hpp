#pragma once

#include <asx/modbus_rtu.hpp>

#include "datagram.hpp"
#include "conf_uart.hpp"


namespace relay {
   // All APIs declared in datagram.hpp

   // Our relay modbus rtu slave templated class
   using modbus_slave = asx::modbus::Slave<Datagram, board::Uart>;

} // End of namespace relay

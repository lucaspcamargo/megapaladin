// this core controls power led, pio and rt stuff
// can talk to main core via a couple of ring buffers

void setup1()
{
  pinMode(PIN_OUT_PWR_LED, OUTPUT);
  digitalWrite(PIN_OUT_PWR_LED, HIGH);
}

void loop1()
{
  FIFOCmd core0_cmd;
  if(rp2040.fifo.pop_nb(reinterpret_cast<uint32_t*>(&core0_cmd)))
  {
    switch(core0_cmd.opcode)
    {
      case FC_STATUS_REQ:
        {
          FIFOCmd reply;
          reply.opcode = FC_STATUS_REPL;
          rp2040.fifo.push(*reinterpret_cast<uint32_t*>(&reply));      
        }
        break;
      case FC_POWER_LED_BLINK:
        {
          // TODO do this with a state machine in the loop, without blocking
          uint8_t times = core0_cmd.data[0];
          while(times)
          {
            digitalWrite(PIN_OUT_PWR_LED, LOW);
            delay(BLINK_DURATION_MS);  
            digitalWrite(PIN_OUT_PWR_LED, HIGH);
            if(times)
              delay(BLINK_INTERVAL_MS);
          }
        }
        break;
    }
  }
}

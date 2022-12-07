// #include "transmit.h"

// #include "mbed.h"
// #include "radio.h"

// #include "config.h"

// EventQueue queue(4 * EVENTS_EVENT_SIZE);

// Thread radioThread;

// void transmit(uint8_t *data) {
//     Radio::radio.tx_buf[0] = *data;
//     Radio::Send(1, 0, 0, 0);
// }

// void queue_transmit(uint8_t data)
// {
//     queue.call(callback(transmit, &data));
// }

// void txDoneCB()
// {
// }

// void rxDoneCB(uint8_t size, float rssi, float snr)
// {
//     puts("got response: ");

//     for (size_t i = 0; i < size; i++) {
//         printf("%c", Radio::radio.rx_buf[i]);
//     }

//     puts("\r\n");
// }


// void radio_irq_callback()
// {
//     queue.call(Radio::service);
// }


// const RadioEvents_t rev = {
//     /* DioPin_top_half */   radio_irq_callback,
//     /* TxDone_topHalf */    NULL,
//     /* TxDone_botHalf */    txDoneCB,
//     /* TxTimeout  */        NULL,
//     /* RxDone  */           rxDoneCB,
//     /* RxTimeout  */        NULL,
//     /* RxError  */          NULL,
//     /* FhssChangeChannel  */NULL,
//     /* CadDone  */          NULL
// };

// void start_LoRa() {
//     printf("init LoRa\r\n");

//     Radio::Init(&rev);

//     Radio::Standby();
//     Radio::LoRaModemConfig(BW_KHZ, SPREADING_FACTOR, 1);
//     Radio::SetChannel(CF_HZ);

//     Radio::set_tx_dbm(TX_DBM);

//                // preambleLen, fixLen, crcOn, invIQ
//     Radio::LoRaPacketConfig(8, false, true, false);

//     radioThread.start([]() {
//         queue.dispatch_forever();
//     });
// }
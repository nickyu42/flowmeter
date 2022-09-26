#include "radio.h"
#include "config.h"
#include "master.h"

/**********************************************************************/
EventQueue queue(4 * EVENTS_EVENT_SIZE);

void tx_test()
{
    static uint8_t seq = 0;

    Radio::radio.tx_buf[0] = seq++;  /* set payload */
    Radio::Send(1, 0, 0, 0);   /* begin transmission */
    printf("sent\r\n");

/*    {
        mbed_stats_cpu_t stats;
        mbed_stats_cpu_get(&stats);
        printf("canDeep:%u ", sleep_manager_can_deep_sleep());
        printf("Uptime: %llu ", stats.uptime / 1000);
        printf("Sleep time: %llu ", stats.sleep_time / 1000);
        printf("Deep Sleep: %llu\r\n", stats.deep_sleep_time / 1000);
    }*/
}

void txDoneCB()
{
    printf("got-tx-done\r\n");
    queue.call_in(500, tx_test);
}

void rxDoneCB(uint8_t size, float rssi, float snr)
{
}


void radio_irq_callback()
{
    queue.call(Radio::service);
}


const RadioEvents_t rev = {
    /* DioPin_top_half */   radio_irq_callback,
    /* TxDone_topHalf */    NULL,
    /* TxDone_botHalf */    txDoneCB,
    /* TxTimeout  */        NULL,
    /* RxDone  */           rxDoneCB,
    /* RxTimeout  */        NULL,
    /* RxError  */          NULL,
    /* FhssChangeChannel  */NULL,
    /* CadDone  */          NULL
};

void master_main()
{
    printf("\r\nreset-tx ");

    Radio::Init(&rev);

    Radio::Standby();
    Radio::LoRaModemConfig(BW_KHZ, SPREADING_FACTOR, 1);
    Radio::SetChannel(CF_HZ);

    Radio::set_tx_dbm(TX_DBM);

               // preambleLen, fixLen, crcOn, invIQ
    Radio::LoRaPacketConfig(8, false, true, false);

    queue.call_in(500, tx_test);

    queue.dispatch();
}


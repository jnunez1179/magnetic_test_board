#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <pb_decode.h>
#include <pb_encode.h>
#include "can_link.h"
#include "mtb.pb.h"
#include "link.pb.h"

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

K_SEM_DEFINE(mtb_sem, 0, 1);

/* Frequency Defines */
#define SAMPLING_HZ      3600000       /*3.6MHZ (will be used in main.c during  ADC transfer)*/
#define AMOUNT_FREQUENCIES 15
#define MAX_SAMPLES        540

/* Broadcast Defines*/
#define OBC        0
#define BROADCAST  1

/*
 * Payload Metadata Template structure
 * Used to specify different message types: DATA/HEALTH/STATUS
 * */

typedef struct  {
  MtbPayload_MsgType type;

  uint8_t target_node;
  uint8_t priority;

  uint8_t record_count;   /*Amount of Frequency Tests recorded: 15*/

  bool broadcast;
} MtbData;

/*TODO: Move FrequencyResults to main.c to be stored/accessed from FLASH*/
/*Parameters used to formulate the different frequency results*/
typedef struct FrequencyResults  {
  uint32_t sampling_hz;
  uint32_t frequency_mhz[AMOUNT_FREQUENCIES];

  uint32_t time_start_us[AMOUNT_FREQUENCIES];
  uint32_t time_end_us[AMOUNT_FREQUENCIES];
  uint32_t time_shift_ns[AMOUNT_FREQUENCIES];

  uint16_t voltage_adc[AMOUNT_FREQUENCIES][MAX_SAMPLES];
  uint16_t voltage_count[AMOUNT_FREQUENCIES];
  uint16_t current_adc[AMOUNT_FREQUENCIES][MAX_SAMPLES];
  uint16_t current_count[AMOUNT_FREQUENCIES];
} FrequencyResults;

static void build_mtb_data(MtbData *mtb_variant, MtbPayload_MsgType type, uint8_t target_node,
    uint8_t priority, bool broadcast)
{
  mtb_variant->type = type;
  mtb_variant->target_node = target_node;
  mtb_variant->priority = priority;
  mtb_variant->broadcast = broadcast;
  mtb_variant->record_count = AMOUNT_FREQUENCIES;
}

/*Fill each corresponding frequency range with metadata and measured data*/
static void build_frequency_records(MtbPayload *mtb_payload, const FrequencyResults *frequency_results) {
  for (uint8_t i = 0; i < AMOUNT_FREQUENCIES; i++) {
    mtb_payload->records[i].frequency_mhz = frequency_results->frequency_mhz[i];
    mtb_payload->records[i].sampling_hz = frequency_results->sampling_hz;

    mtb_payload->records[i].time_start_us = frequency_results->time_start_us[i];
    mtb_payload->records[i].time_end_us   = frequency_results->time_end_us[i];
    mtb_payload->records[i].time_shift_ns = frequency_results->time_shift_ns[i];

    for (uint16_t j = 0; j < MAX_SAMPLES; j++) {
      mtb_payload->records[i].voltage_adc[j] = frequency_results->voltage_adc[i][j];
      mtb_payload->records[i].current_adc[j] = frequency_results->current_adc[i][j];
    }

    mtb_payload->records[i].voltage_adc_count = frequency_results->voltage_count[i];
    mtb_payload->records[i].current_adc_count = frequency_results->current_count[i];
  }
}

/*Encode Payload*/
static bool encode_mtb_payload(MtbPayload *mtb_payload, MtbData *mtb_data, uint32_t seq,
                        uint8_t *buffer, size_t *encoded_len)
{
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, MtbPayload_size);
  mtb_payload->node_id = can_link_node_id();
  mtb_payload->seq = seq;

  mtb_payload->type = mtb_data->type;

  mtb_payload->records_count = mtb_data->record_count;

  mtb_payload->target_node = mtb_data->target_node;
  mtb_payload->priority = mtb_data->priority;
  mtb_payload->uptime_ms = k_uptime_get_32();

  if (!pb_encode(&stream, MtbPayload_fields, mtb_payload)) {
    LOG_ERR("Nanopb encode failed: %s", PB_GET_ERROR(&stream));
    return false;
  }

  *encoded_len = stream.bytes_written;
  return true;
}

static bool decode_message(const uint8_t *buffer, size_t len, LinkMessage *msg)
{
    pb_istream_t stream = pb_istream_from_buffer(buffer, len);

    *msg = (LinkMessage)LinkMessage_init_zero;
    if (!pb_decode(&stream, LinkMessage_fields, msg)) {
        LOG_ERR("Nanopb decode failed: %s", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

static void on_can_message(const uint8_t *buffer, size_t len, uint8_t source_node,
               bool is_broadcast, void *user_data)
{
  LinkMessage msg;

  if (!decode_message(buffer, len, &msg)) {
      return;
  }

  LOG_INF("RX [Prio:%u] src:%u->dst:%u type:%u seq:%" PRIu32 " (BC:%d)",
      (unsigned int)msg.priority, source_node, (unsigned int)msg.target_node,
      (unsigned int)msg.type, msg.seq, is_broadcast);

  if (msg.type == LinkMessage_MsgType_START_TEST) {
    k_sem_give(&mtb_sem);
    return;
  }
}


/*Protobuf message transmit buffer*/
uint8_t tx_buffer[MtbPayload_size];

/*Structure containing Results from MTB Test on DUT (voltage/current & timestamps/frequency)*/
FrequencyResults frequency_results;

/*Configuration settings of each Protobuf payload*/
MtbData mtb_data;

/*Generalized access point to mtb_data*/
MtbData *action;

/*Final Protobuf payload to be sent to OBC*/
MtbPayload mtb_payload;



  int ret = can_link_init(on_can_message, NULL);
  if (ret != 0) {
    LOG_ERR("CAN init failed: %d", ret);
    return 0;
  }





  while (1) {
    k_sem_take(&mtb_sem, K_FOREVER);


    build_mtb_data(&mtb_data, MtbPayload_MsgType_DATA, CONFIG_CAN_LINK_PEER_NODE_ADDR, 0U, OBC);
    action = &mtb_data;



    if (action->broadcast) {
        ret = can_link_send_broadcast(tx_buffer, encoded_len);
    } else {
        ret = can_link_send_to(action->target_node, tx_buffer, encoded_len);
    }

  return 0;
}










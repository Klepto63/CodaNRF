#include "drv_helmet.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> 
#include "mdw_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#include "nordic_common.h"
#include "param_global.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_pwr_mgmt.h"
#include "peer_manager.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "fds.h"
#include "nrf_fstorage.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "ble_nus_c.h"
#include "nrf_sdh_freertos.h"


static void helmet_task(void *pArg);
static void rtos_init(void);
static TaskHandle_t         HelmettaskHandle;

#define APP_BLE_CONN_CFG_TAG        1                                   /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_BLE_OBSERVER_PRIO       3                                   /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_SOC_OBSERVER_PRIO       1                                   /**< Applications' SoC observer priority. You shoulnd't need to modify this value. */
#define SEC_PARAM_BOND              1                                   /**< Perform bonding. */
#define SEC_PARAM_MITM              0                                   /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC              0                                   /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS          0                                   /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES   BLE_GAP_IO_CAPS_NONE                /**< No I/O capabilities. */
#define SEC_PARAM_OOB               0                                   /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE      7                                   /**< Minimum encryption key size in octets. */
#define SEC_PARAM_MAX_KEY_SIZE      16                                  /**< Maximum encryption key size in octets. */
#define SCAN_INTERVAL               0x00A0                              /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                 0x0050                              /**< Determines scan window in units of 0.625 millisecond. */
#define NUS_SERVICE_UUID_TYPE       BLE_UUID_TYPE_VENDOR_BEGIN          /**< UUID type for the Nordic UART Service (vendor specific). */
#define MIN_CONNECTION_INTERVAL     MSEC_TO_UNITS(7.5, UNIT_1_25_MS)    /**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(30, UNIT_1_25_MS)     /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY               0                                   /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT         MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 millisecond. */
#define TARGET_UUID                 BLE_UUID_HEART_RATE_SERVICE         /**< Target device name that application is looking for. */
static bool erase_bonds = false;

static t_cb                   event_cb;
static TaskHandle_t           helmetTaskHandle;
static EventGroupHandle_t     helmetFlagHandle;

static EventBits_t helmetFlagDataReady   = 0x0002;

#define HELMET_TRAME_MIN_DATA_LENGTH     34
#define HELMET_FRAM_MAX_SIZE             40  

uint8_t RXBuffer[HELMET_FRAM_MAX_SIZE] = {0};
uint8_t RXlength = 0; 
bool    mutexInCopy = false;
bool    streamStarded = false;
bool    dataReady = false; //quickbugfix instead of flaghandle asserting

s_accValue curHelmetAccValue = {0};


/**@brief Variable length data encapsulation in terms of length and pointer to data */
typedef struct
{
    uint8_t  * p_data;      /**< Pointer to data. */
    uint16_t   data_len;    /**< Length of data. */
} data_t;

BLE_NUS_C_DEF(m_ble_nus_c);                                             /**< BLE NUS service client instance. */
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
BLE_DB_DISCOVERY_DEF(m_db_disc);                                    /**< DB discovery module instance. */

static uint16_t m_conn_handle;                                      /**< Current connection handle. */
static bool     m_whitelist_disabled;                               /**< True if whitelist has been temporarily disabled. */
static bool     m_memory_access_in_progress;                        /**< Flag to keep track of ongoing operations on persistent memory. */

static ble_gap_scan_params_t m_scan_param;                          /**< Scan parameters requested for scanning and connection. */

/**@brief Connection parameters requested for connection. */
static ble_gap_conn_params_t const m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,  /**< Minimum connection. */
    (uint16_t)MAX_CONNECTION_INTERVAL,  /**< Maximum connection. */
    (uint16_t)SLAVE_LATENCY,            /**< Slave latency. */
    (uint16_t)SUPERVISION_TIMEOUT       /**< Supervision time-out. */
};

static char const m_target_periph_name[] = "RN_BLE";      /**< If you want to connect to a peripheral using a given advertising name, type its name here. */

static void rtos_init(void)
{
    BaseType_t xResult = xTaskCreate(helmet_task,
                        "HEL",
                        configMINIMAL_STACK_SIZE + 100,
                        NULL,
                        1,
                        &helmetTaskHandle);
    ASSERT(NULL != helmetTaskHandle);

    EventGroupHandle_t helmetFlagHandle = xEventGroupCreate();
    ASSERT(NULL != helmetFlagHandle);
}

static void scan_start(void);
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

uint8_t drv_helmet_get(s_accValue accValues);
s_accValue deserialize(s_accValue* phelmetData, uint8_t* pdata, uint8_t RXlength);

static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_nus_c_on_db_disc_evt(&m_ble_nus_c, p_evt);
    switch(p_evt->evt_type)
    {
        case BLE_DB_DISCOVERY_SRV_NOT_FOUND : 
        {
            DEBUG("service not found");
        } break;
        case BLE_DB_DISCOVERY_COMPLETE : 
        {
            DEBUG("Discovery complete");
        } break;
        default : 
        {
            DEBUG_LOG("db_disc_handler %d", p_evt->evt_type);
        }
    }
}

static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
        {
            DEBUG("Connected to a previously bonded device.");
        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            DEBUG_LOG("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
                         ble_conn_state_role(p_evt->conn_handle),
                         p_evt->conn_handle,
                         p_evt->params.conn_sec_succeeded.procedure);
        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                ASSERT(0 == err_code);
            }
        } break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {
            // Bonds are deleted. Start scanning.
            scan_start();
        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
            ASSERT(0 == p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
            ASSERT(0 == p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
            ASSERT(0 == p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
            ASSERT(0 == p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}

/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t  index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index + 1];

        if (field_type == type)
        {
            p_typedata->p_data   = &p_data[index + 2];
            p_typedata->data_len = field_length - 1;
            return NRF_SUCCESS;
        }
        index += field_length + 1;
    }
    return NRF_ERROR_NOT_FOUND;
}
/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook( void )
{
    
}

static bool find_adv_name(ble_gap_evt_adv_report_t const * p_adv_report, char const * name_to_find)
{
    ret_code_t err_code;
    data_t     adv_data;
    data_t     dev_name;
    // Initialize advertisement report for parsing
    adv_data.p_data   = (uint8_t *)p_adv_report->data;
    adv_data.data_len = p_adv_report->dlen;
    //search for advertising names
    err_code = adv_report_parse(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, &adv_data, &dev_name);
    if (err_code == NRF_SUCCESS)
    {
        if (memcmp(name_to_find, dev_name.p_data, dev_name.data_len) == 0)
        {
            return true;
        }
    }
    else
    {
        // Look for the short local name if it was not found as complete
        err_code = adv_report_parse(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, &adv_data, &dev_name);
        if (err_code != NRF_SUCCESS)
        {
            return false;
        }
        if (memcmp(m_target_periph_name, dev_name.p_data, dev_name.data_len )== 0)
        {
            return true;
        }
    }
    return false;
}

static bool find_peer_addr(const ble_gap_evt_adv_report_t *p_adv_report, const ble_gap_addr_t * p_addr)
{
    if (p_addr->addr_type == p_adv_report->peer_addr.addr_type)
    {
        if (memcmp(p_addr->addr, p_adv_report->peer_addr.addr, sizeof(p_adv_report->peer_addr.addr)) == 0)
        {
            return true;
        }
    }
    return false;
}

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {
            DEBUG("BLEcb-> Connected.");
            // Discover peer's services.
            err_code = ble_db_discovery_start(&m_db_disc, p_ble_evt->evt.gap_evt.conn_handle);
            ASSERT(0 == err_code);
        } break;

        case BLE_GAP_EVT_ADV_REPORT:
        {
            bool do_connect = false;
            if (find_adv_name(&p_gap_evt->params.adv_report, m_target_periph_name))
            {
                do_connect = true;
                DEBUG("Name match send connect_request.");
            }
            if (do_connect)
            {
                // Stop scanning.
                (void) sd_ble_gap_scan_stop();

                #if (NRF_SD_BLE_API_VERSION <= 2)
                    m_scan_param.selective = 0;
                #endif
                #if (NRF_SD_BLE_API_VERSION >= 3)
                    m_scan_param.use_whitelist = 0;
                #endif

                // Initiate connection.
                err_code = sd_ble_gap_connect(&p_gap_evt->params.adv_report.peer_addr,
                                              &m_scan_param,
                                              &m_connection_param,
                                              APP_BLE_CONN_CFG_TAG);

                m_whitelist_disabled = false;

                if (err_code != NRF_SUCCESS)
                {
                    DEBUG_LOG("Conne Failed, reason %d.", err_code);
                }
            }
        } break; // BLE_GAP_EVT_ADV_REPORT

        case BLE_GAP_EVT_DISCONNECTED:
        {
            DEBUG("BLEcb-> disconnected.");
            DEBUG_LOG("Disconnected, reason 0x%x.",
                         p_ble_evt->evt.gap_evt.params.disconnected.reason);
            if (ble_conn_state_n_centrals() < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
            {
                //scan_start(); //todo remettre
            }
        } break;

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE :
        {
            DEBUG("-> data length updated");
        }  break;


        case BLE_GAP_EVT_TIMEOUT:
        {
             DEBUG("BLEcb-> GAP Timeout.");
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                DEBUG("Scan timed out.");
                scan_start();
            }
            else if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                DEBUG("Connection Request timed out.");
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            DEBUG("BLEcb-> Param update req.");
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            //ASSERT(0 == err_code); //todo
            break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            ASSERT(0 == err_code);
        } break;
#endif

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            DEBUG("BLEcb-> GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            ASSERT(0 == err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            DEBUG("BLEcb-> GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            ASSERT(0 == err_code);
            break;

        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP:
        {
            DEBUG("-> MTU changed");
        } break;

        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
        {
            DEBUG("-> Gatt evt PRIMARY SERVICE RSP");
        }break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
        {
        } break;

        case BLE_GATTC_EVT_DESC_DISC_RSP : 
        {
            DEBUG("-> Desc discove rsp event");
        } break;

        case BLE_GATTC_EVT_HVX :
        {
            //
        } break;

        default:
            DEBUG_LOG("-> Unkown %d", p_ble_evt->header.evt_id);

            if(p_ble_evt->header.evt_id == 18)
            {
                uint8_t i = 69;
                i++;
            }

            break;
    }
}

static void soc_evt_handler(uint32_t evt_id, void * p_context)
{
    switch (evt_id)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            /* fall through */
        case NRF_EVT_FLASH_OPERATION_ERROR:

            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                scan_start();
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}


static void ble_stack_init(void)
{
    ret_code_t err_code;
    err_code = nrf_sdh_enable_request();
    ASSERT(0 == err_code);
    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    ASSERT(0 == err_code);
    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    ASSERT(0 == err_code);
    // Register handlers for BLE and SoC events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);
}

static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    ASSERT(0 == err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    ASSERT(0 == err_code);

    err_code = pm_register(pm_evt_handler);
    ASSERT(0 == err_code);
}


static void delete_bonds(void)
{
    ret_code_t err_code;
    DEBUG("Erase bonds!");
    err_code = pm_peers_delete();
    ASSERT(0 == err_code);
}

static void whitelist_disable(void)
{
    if (!m_whitelist_disabled)
    {
        DEBUG("Whitelist temporarily disabled.");
        m_whitelist_disabled = true;
        (void) sd_ble_gap_scan_stop();
        scan_start();
    }
}

/**
 * @brief Database discovery collector initialization.
 */
static void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    ASSERT(0 == err_code);
}

/**@brief Retrieve a list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
    pm_peer_id_t peer_id;
    uint32_t     peers_to_copy;

    peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
                     *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    *p_size = 0;
    while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
    {
        p_peers[(*p_size)++] = peer_id;
        peer_id = pm_next_peer_id_get(peer_id);
    }
}

static void whitelist_load()
{
    ret_code_t   ret;
    pm_peer_id_t peers[8];
    uint32_t     peer_cnt;

    memset(peers, PM_PEER_ID_INVALID, sizeof(peers));
    peer_cnt = (sizeof(peers) / sizeof(pm_peer_id_t));

    // Load all peers from flash and whitelist them.
    peer_list_get(peers, &peer_cnt);
    ret = pm_whitelist_set(peers, peer_cnt);
    ASSERT(0 == ret);
    // Setup the device identies list.
    // Some SoftDevices do not support this feature.
    ret = pm_device_identities_list_set(peers, peer_cnt);
    if (ret != NRF_ERROR_NOT_SUPPORTED)
    {
        ASSERT(0 == ret);
    }
}


static void scan_start(void)
{
    if (nrf_fstorage_is_busy(NULL))
    {
        m_memory_access_in_progress = true;
        return;
    }

    // Whitelist buffers.
    ble_gap_addr_t whitelist_addrs[8];
    ble_gap_irk_t  whitelist_irks[8];

    memset(whitelist_addrs, 0x00, sizeof(whitelist_addrs));
    memset(whitelist_irks,  0x00, sizeof(whitelist_irks));

    uint32_t addr_cnt = (sizeof(whitelist_addrs) / sizeof(ble_gap_addr_t));
    uint32_t irk_cnt  = (sizeof(whitelist_irks)  / sizeof(ble_gap_irk_t));

    #if (NRF_SD_BLE_API_VERSION <= 2)

        ble_gap_addr_t * p_whitelist_addrs[8];
        ble_gap_irk_t  * p_whitelist_irks[8];

        for (uint32_t i = 0; i < 8; i++)
        {
            p_whitelist_addrs[i] = &whitelist_addrs[i];
            p_whitelist_irks[i]  = &whitelist_irks[i];
        }

        ble_gap_whitelist_t whitelist =
        {
            .pp_addrs = p_whitelist_addrs,
            .pp_irks  = p_whitelist_irks,
        };

    #endif

    // Reload the whitelist and whitelist all peers.
    whitelist_load();

    ret_code_t ret;

    // Get the whitelist previously set using pm_whitelist_set().
    ret = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                           whitelist_irks,  &irk_cnt);

    m_scan_param.active   = 0;
    m_scan_param.interval = SCAN_INTERVAL;
    m_scan_param.window   = SCAN_WINDOW;

    if (((addr_cnt == 0) && (irk_cnt == 0)) ||
        (m_whitelist_disabled))
    {
        // Don't use whitelist.
        #if (NRF_SD_BLE_API_VERSION <= 2)
            m_scan_param.selective   = 0;
            m_scan_param.p_whitelist = NULL;
        #endif
        #if (NRF_SD_BLE_API_VERSION >= 3)
            m_scan_param.use_whitelist  = 0;
            m_scan_param.adv_dir_report = 0;
        #endif
        m_scan_param.timeout  = 0x0000; // No timeout.
    }
    else
    {
        // Use whitelist.
        #if (NRF_SD_BLE_API_VERSION <= 2)
            whitelist.addr_count     = addr_cnt;
            whitelist.irk_count      = irk_cnt;
            m_scan_param.selective   = 1;
            m_scan_param.p_whitelist = &whitelist;
        #endif
        #if (NRF_SD_BLE_API_VERSION >= 3)
            m_scan_param.use_whitelist  = 1;
            m_scan_param.adv_dir_report = 0;
        #endif
        m_scan_param.timeout  = 0x001E; // 30 seconds.
    }

    DEBUG("Starting scan.");
    ret = sd_ble_gap_scan_start(&m_scan_param);
    ASSERT(0 == ret);
}

static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    switch (p_evt->evt_id)
    {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        {
            DEBUG_LOG("GATT ATT MTU on connection 0x%x changed to %d.",
                         p_evt->conn_handle,
                         p_evt->params.att_mtu_effective);
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        {
            DEBUG_LOG("Data length for connection 0x%x updated to %d.",
                         p_evt->conn_handle,
                         p_evt->params.data_length);
        } break;

        default:
            break;
    }
}

static void ble_nus_c_evt_handler(ble_nus_c_t * p_ble_nus_c, ble_nus_c_evt_t const * p_ble_nus_evt)
{
    ret_code_t err_code;
    switch (p_ble_nus_evt->evt_type)
    {
        case BLE_NUS_C_EVT_DISCOVERY_COMPLETE:
            DEBUG("Discovery complete.");
            err_code = ble_nus_c_handles_assign(p_ble_nus_c, p_ble_nus_evt->conn_handle, &p_ble_nus_evt->handles);
            ASSERT(0 == err_code);
            err_code = ble_nus_c_tx_notif_enable(p_ble_nus_c);
            ASSERT(0 == err_code);
            break;

        case BLE_NUS_C_EVT_NUS_TX_EVT:

            if(p_ble_nus_evt->data_len < HELMET_TRAME_MIN_DATA_LENGTH)
            {
                break;//pas une trame data
            }
            if(mutexInCopy)
            {
                DEBUG("hel: skipped frame mutex taken");
                return; //skip a frame
            }
            mutexInCopy = true;
            memcpy(RXBuffer, p_ble_nus_evt->p_data, p_ble_nus_evt->data_len);
            RXlength = p_ble_nus_evt->data_len;
            mutexInCopy = false;    

            if(p_ble_nus_evt->data_len > 30)
            {
                streamStarded = true;
            } 

            dataReady = true;
           //Bug SetBitsFromISR asserting... Quick bugfix by using global var
           // BaseType_t xHigherPriorityTaskWoken, xResult;
           // xHigherPriorityTaskWoken = pdFALSE;
           // xResult = xEventGroupSetBitsFromISR(helmetFlagHandle, helmetFlagDataReady, &xHigherPriorityTaskWoken);
           // if( xResult != pdFAIL )
           // {
           //     /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
           //     switch should be requested.  The macro used is port specific and will
           //     be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
           //     the documentation page for the port being used. */
           //     portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
           // }
           //
            break;

        case BLE_NUS_C_EVT_DISCONNECTED:
            DEBUG("Disconnected.");
            scan_start();
            break;
    }
}

/**@brief Function for initializing the NUS Client. */
static void nus_c_init(void)
{
    ret_code_t       err_code;
    ble_nus_c_init_t init;
    init.evt_handler = ble_nus_c_evt_handler;
    err_code = ble_nus_c_init(&m_ble_nus_c, &init);
    ASSERT(0 == err_code);
}

void drv_helmet_init(t_cb cb)
{
    event_cb = cb; 
    rtos_init();
    ble_stack_init();
    nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    peer_manager_init();
    db_discovery_init();

    vTaskDelay(100);
    if (erase_bonds == true)
    {
        delete_bonds();   
    }
    else
    {
        scan_start();
    }
    nus_c_init();
    nrf_sdh_freertos_init(NULL, false);
}

uint8_t drv_helmet_acc_read(s_accValue* accValues)
{
    while(mutexInCopy){}
    uint8_t internalRXBuffer[HELMET_FRAM_MAX_SIZE] = {0};
    uint8_t internalLength = RXlength;
    memcpy(internalRXBuffer, RXBuffer, RXlength); //for data corruption

    uint8_t tempBuffer[3][10] = {0};
    uint8_t idx, idy = 0;

    if(internalRXBuffer[0] != 0x23)
    {
        return 1; //parse error
    }

    for(uint8_t i = 1; i < RXlength; i++)
    {
        if(internalRXBuffer[i] != 0x2C)
        {
            tempBuffer[idy][idx] = internalRXBuffer[i];
            idx++;
        }
        else
        {
            idx = 0;
            idy ++;
            if(idy == 3)
            {
                break;
            }
        }
    }

    accValues->accPitch = atof(tempBuffer[0]);
    accValues->accRoll = atof(tempBuffer[1]);
    accValues->accHeading = atof(tempBuffer[2]);
    return 0;
}

bool drv_helmet_stream_started()
{
    return streamStarded;
}

static void helmet_task(void *pArg)
{
    while(!streamStarded)
    {
        vTaskDelay(1);
    }

    DEBUG("Helmet task stream detected.");

    while(1)
    {
        while(!dataReady) //quickbugfix instead of flaghandle asserting
        {
            vTaskDelay(1);
        }
        dataReady = false;
        event_cb(e_helmetEventRxEvent, 0, 0);   
    }
}


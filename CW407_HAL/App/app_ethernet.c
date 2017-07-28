/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/app_ethernet.c 
  * @author  MCD Application Team
  * @version V1.0.2
  * @date    06-May-2016
  * @brief   Ethernet specefic module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright © 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "main.h"
#include "lwip/dhcp.h"
#include "app_ethernet.h"
#include "ethernetif.h"
#include "config.h"
#include "lwip.h"
#include "api.h"
#include "tcp.h"
#include "netbuf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef USE_DHCP
#define MAX_DHCP_TRIES  5
__IO uint8_t DHCP_state;
#endif

/* Private function prototypes -----------------------------------------------*/
static void tcpecho_thread(void *arg);
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
void User_notification(struct netif *netif)
{
   if (netif_is_up(netif))
   {
#ifdef USE_DHCP
      /* Update DHCP state machine */
      DHCP_state = DHCP_START;
#endif /* USE_DHCP */
   }
   else
   {
#ifdef USE_DHCP
      /* Update DHCP state machine */
      DHCP_state = DHCP_LINK_DOWN;
#endif  /* USE_DHCP */
   }
}

#ifdef USE_DHCP
/**
* @brief  DHCP Process
* @param  argument: network interface
* @retval None
*/
void DHCP_thread(void const *argument)
{
   struct netif *netif = (struct netif *)argument;
   ip_addr_t ipaddr;
   ip_addr_t netmask;
   ip_addr_t gw;
   uint32_t IPaddress;
   char str_tmp[50];
   uint8_t temp[4];

   DEBUG_PrintfString("DHCP thread startup.\r\n");

   for (;;)
   {
      switch (DHCP_state)
      {
      case DHCP_START:
         {
            netif->ip_addr.addr = 0;
            netif->netmask.addr = 0;
            netif->gw.addr = 0;
            IPaddress = 0;
            dhcp_start(netif);
            DHCP_state = DHCP_WAIT_ADDRESS;
            DEBUG_PrintfString("DHCP start.\r\nwaitting address.\r\n");
         }
         break;

      case DHCP_WAIT_ADDRESS:
         {
            /* Read the new IP address */
            IPaddress = netif->ip_addr.addr;

            if (IPaddress != 0)
            {
               DHCP_state = DHCP_ADDRESS_ASSIGNED;
               DEBUG_PrintfString("DHCP address assigned.\r\n");

               temp[0] = (uint8_t)(IPaddress >> 24);
               temp[1] = (uint8_t)(IPaddress >> 16);
               temp[2] = (uint8_t)(IPaddress >> 8);
               temp[3] = (uint8_t)(IPaddress);
               sprintf(str_tmp, "local ip:%d.%d.%d.%d\r\n", temp[3], temp[2], temp[1], temp[0]);
               DEBUG_PrintfString(str_tmp);

               IPaddress = netif->gw.addr;
               temp[0] = (uint8_t)(IPaddress >> 24);
               temp[1] = (uint8_t)(IPaddress >> 16);
               temp[2] = (uint8_t)(IPaddress >> 8);
               temp[3] = (uint8_t)(IPaddress);
               sprintf(str_tmp, "gate:%d.%d.%d.%d\r\n", temp[3], temp[2], temp[1], temp[0]);
               DEBUG_PrintfString(str_tmp);

               IPaddress = netif->netmask.addr;
               temp[0] = (uint8_t)(IPaddress >> 24);
               temp[1] = (uint8_t)(IPaddress >> 16);
               temp[2] = (uint8_t)(IPaddress >> 8);
               temp[3] = (uint8_t)(IPaddress);
               sprintf(str_tmp, "net mask:%d.%d.%d.%d\r\n", temp[3], temp[2], temp[1], temp[0]);
               DEBUG_PrintfString(str_tmp);

               /* Stop DHCP */
//               dhcp_stop(netif);
            }
            else
            {
               /* DHCP timeout */
               if (netif->dhcp->tries > MAX_DHCP_TRIES)
               {
                  DHCP_state = DHCP_TIMEOUT;
                  DEBUG_PrintfString("Time out.\r\nDHCP stop.\r\nUse static ip\r\n");
                  /* Stop DHCP */
                  dhcp_stop(netif);

                  /* Static address used */
                  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
                  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
                  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
                  netif_set_addr(netif, &ipaddr, &netmask, &gw);

               }
            }
         }
         break;

      default:
         break;
      }

      /* wait 250 ms */
      osDelay(250);
   }
}
#endif  /* USE_DHCP */

#ifdef LWIP_EN
/**
* @brief  TCP Client Process
* @param  argument: network interface
* @retval None
*/
void TCP_Client_thread(void const *argument)
{
   struct netif *netif = (struct netif *)argument;
//   ip_addr_t ipaddr;
//   ip_addr_t netmask;
//   ip_addr_t gw;
//   uint32_t IPaddress;

   struct netconn *conn, *newconn;
   static ip_addr_t ipaddr;
   struct netbuf *buf;
   err_t err1, err2;
   uint8_t i;

   static char str_tmp[100];

   DEBUG_PrintfString("TCP Client thread startup.\r\n");

   /* µÈ´ýÍø¿ÚÁ¬½ÓÉÏ£¬³É¹¦»ñÈ¡IP */
   while (DHCP_state != DHCP_ADDRESS_ASSIGNED)
   {
      osDelay(500);
   }


   for (i = 0; i < 5; i++)
   {
      osDelay(500);
      DEBUG_PrintfString("connecting sever..\r\n");

      conn = netconn_new(NETCONN_TCP);
      if (conn == NULL)
      {
         DEBUG_PrintfString("creat a connected fail.\r\n");
         continue;
      }
      //	addr.addr = htonl(0xc0a80164);

      IP4_ADDR(&ipaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);
      err1 = netconn_bind(conn, &netif->ip_addr, 0);
//    err1 = netconn_bind(conn, NULL, 2000);
      err2 = netconn_connect(conn, &ipaddr, DEST_PORT);
      
      if (err1 != ERR_OK || err2 != ERR_OK)
      {
         DEBUG_PrintfString("connect sever fail.\r\n");
         netconn_delete(conn);
         continue;
      }
      else
      {
         DEBUG_PrintfString("connect sever sucess.\r\n");
         break;
      }
   }
   
   netconn_write(conn, "start tcp\r\n", 11, NETCONN_NOCOPY);

   /*connect to tcp server */
//  tcp_echoclient_connect();

//   tcpecho_thread(NULL);

//   DEBUG_PrintfString("Waitting connect..\r\n");
//   conn = netconn_new(NETCONN_TCP);
//   netconn_bind(conn, NULL, 2000);
//   netconn_listen(conn);
//   netconn_accept(conn, &newconn);

   for (;;)
   {
//      err = netconn_recv(newconn, &buf);  // ÔÚÐÂÁ¬½ÓÉÏ½ÓÊÕµ½Ò»¸öÊý¾Ý
//  netbuf_delete(buf);           // É¾³ý½ÓÊÕµ½µÄÊý¾Ý
      if (err1 == ERR_OK)
      {
//         netconn_write(newconn, data, sizeof(data), NETCONN_COPY); // ½«×Ö·û´®·¢ËÍµÄ¿Í»§¶Ë
      }
      /* wait 250 ms */
      osDelay(250);
   }
}


static void tcpecho_thread(void *arg)
{
   struct netconn *conn, *newconn;
   err_t err;
   LWIP_UNUSED_ARG(arg);

   /* Create a new connection identifier. */
   conn = netconn_new(NETCONN_TCP);

   /* Bind connection to well known port number 7. */
   netconn_bind(conn, NULL, 2000);

   /* Tell connection to go into listening mode. */
   netconn_listen(conn);

   while (1)
   {

      /* Grab new connection. */
      err = netconn_accept(conn, &newconn);
      /*printf("accepted new connection %p\n", newconn);*/
      /* Process the new connection. */
      if (err == ERR_OK)
      {
         struct netbuf *buf;
         void *data;
         u16_t len;

         while ((err = netconn_recv(newconn, &buf)) == ERR_OK)
         {
            /*printf("Recved\n");*/
            do
            {
               netbuf_data(buf, &data, &len);
               err = netconn_write(newconn, data, len, NETCONN_COPY);
#if 0
               if (err != ERR_OK)
               {
                  printf("tcpecho: netconn_write: error \"%s\"\n", lwip_strerr(err));
               }
#endif
            }
            while (netbuf_next(buf) >= 0);
            netbuf_delete(buf);
         }
         /*printf("Got EOF, looping\n");*/
         /* Close connection and discard connection identifier. */
         netconn_close(newconn);
         netconn_delete(newconn);
      }
   }
}


#endif  /* LWIP */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

#include "fatfs_sd.h"

/* ---- Commandes SD ---- */
#define CMD0    (0)
#define CMD1    (1)
#define ACMD41  (0x80 + 41)
#define CMD8    (8)
#define CMD9    (9)
#define CMD12   (12)
#define CMD16   (16)
#define CMD17   (17)
#define CMD18   (18)
#define ACMD23  (0x80 + 23)
#define CMD24   (24)
#define CMD25   (25)
#define CMD55   (55)
#define CMD58   (58)

static volatile DSTATUS Stat = STA_NOINIT;
static BYTE CardType;

/* ------------------------------------------------------------------ */
/* Primitives SPI bas niveau                                          */
/* ------------------------------------------------------------------ */

static uint8_t SPI_RW(uint8_t dat)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi3, &dat, &rx, 1, 100);
    return rx;
}

static int wait_ready(UINT wt)
{
    uint8_t d;
    uint32_t start = HAL_GetTick();

    do {
        d = SPI_RW(0xFF);
    } while (d != 0xFF && (HAL_GetTick() - start) < wt);

    return (d == 0xFF) ? 1 : 0;
}

static void deselect(void)
{
    SD_CS_HIGH();
    SPI_RW(0xFF);
}

static int select(void)
{
    SD_CS_LOW();
    SPI_RW(0xFF);

    /* CORRECTIF LOGICIEL : Si la carte n'est pas encore identifiée,
       on n'attend pas le signal ready pour éviter un timeout initial */
    if (CardType == 0 || wait_ready(500)) return 1;

    deselect();
    return 0;
}

static int rcvr_datablock(BYTE *buff, UINT btr)
{
    uint8_t token;
    uint32_t start = HAL_GetTick();

    // Attente du token de debut de bloc (0xFE)
    do {
        token = SPI_RW(0xFF);
    } while (token == 0xFF && (HAL_GetTick() - start) < 200);

    if (token != 0xFE) return 0;

    // --- CORRECTIF : On remplace HAL_SPI_Receive par une boucle SPI_RW(0xFF) ---
    // Cela force la ligne MOSI a rester a 0xFF comme l'exige la carte SD
    for (UINT i = 0; i < btr; i++) {
        buff[i] = SPI_RW(0xFF);
    }

    SPI_RW(0xFF); /* CRC dummy 1 */
    SPI_RW(0xFF); /* CRC dummy 2 */

    return 1;
}

static int xmit_datablock(const BYTE *buff, BYTE token)
{
    if (!wait_ready(500)) return 0;

    SPI_RW(token);

    if (token != 0xFD) {
        HAL_SPI_Transmit(&hspi3, (uint8_t*)buff, 512, 200);

        // --- CORRECTIF OBLIGATOIRE : Vidage FIFO & RAZ du Flag OVR ---
        volatile uint8_t dummy;
        while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_RXNE)) {
            dummy = hspi3.Instance->DR;
            (void)dummy;
        }
        __HAL_SPI_CLEAR_OVRFLAG(&hspi3);
        // -------------------------------------------------------------

        SPI_RW(0xFF);
        SPI_RW(0xFF);

        if ((SPI_RW(0xFF) & 0x1F) != 0x05) return 0;
    }
    return 1;
}

static BYTE send_cmd(BYTE cmd, DWORD arg)
{
    BYTE n, res;

    if (cmd & 0x80) { /* ACMD<n> = CMD55 + CMD<n> */
        cmd &= 0x7F;
        res = send_cmd(55, 0);
        if (res > 1) return res;
    }

    deselect();

    if (!select()) return 0xFF;

    uint8_t buf[6];
    buf[0] = 0x40 | cmd;
    buf[1] = (BYTE)(arg >> 24);
    buf[2] = (BYTE)(arg >> 16);
    buf[3] = (BYTE)(arg >> 8);
    buf[4] = (BYTE)(arg);

    if (cmd == 0) buf[5] = 0x95;
    else if (cmd == 8) buf[5] = 0x87;
    else buf[5] = 0x01;

    // --- CORRECTIF : Utilisation de TransmitReceive pour garder la FIFO propre ---
    uint8_t rx_dummy[6];
    HAL_SPI_TransmitReceive(&hspi3, buf, rx_dummy, 6, 100);

    if (cmd == 12) SPI_RW(0xFF); /* octet de remplissage CMD12 */

    n = 10;
    do {
        res = SPI_RW(0xFF);
    } while ((res & 0x80) && --n);

    return res;
}

/* ------------------------------------------------------------------ */
/* Fonctions publiques                                                */
/* ------------------------------------------------------------------ */

DSTATUS SD_disk_initialize(BYTE pdrv)
{
    if (pdrv) return STA_NOINIT;

    BYTE n, cmd, ty, ocr[4];

    SD_CS_HIGH();
    for (n = 10; n; n--) SPI_RW(0xFF); /* >= 74 clocks CS haut */

    ty = 0;

    if (send_cmd(CMD0, 0) == 1)
    {
        uint32_t start = HAL_GetTick();

        if (send_cmd(CMD8, 0x1AA) == 1) /* SD v2 */
        {
            for (n = 0; n < 4; n++) ocr[n] = SPI_RW(0xFF);

            if (ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                while ((HAL_GetTick() - start) < 1000 && send_cmd(ACMD41, 1UL << 30));

                if ((HAL_GetTick() - start) < 1000 && send_cmd(CMD58, 0) == 0)
                {
                    for (n = 0; n < 4; n++) ocr[n] = SPI_RW(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        }
        else /* SD v1 ou MMC */
        {
            if (send_cmd(ACMD41, 0) <= 1) {
                ty = CT_SD1;
                cmd = ACMD41;
            } else {
                ty = CT_MMC;
                cmd = CMD1;
            }

            while ((HAL_GetTick() - start) < 1000 && send_cmd(cmd, 0));

            if ((HAL_GetTick() - start) >= 1000 || send_cmd(CMD16, 512) != 0) ty = 0;
        }
    }

    CardType = ty;
    SD_CS_HIGH();
    SPI_RW(0xFF);

    if (ty) Stat &= ~STA_NOINIT;

    return Stat;
}

DSTATUS SD_disk_status(BYTE pdrv)
{
    if (pdrv) return STA_NOINIT;
    return Stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (!(CardType & CT_BLOCK)) sector *= 512;

    DRESULT res = RES_ERROR;

    if (count == 1) {
        if (send_cmd(CMD17, sector) == 0 && rcvr_datablock(buff, 512)) res = RES_OK;
    } else {
        if (send_cmd(CMD18, sector) == 0) {
            do {
                if (!rcvr_datablock(buff, 512)) break;
                buff += 512;
            } while (--count);

            send_cmd(CMD12, 0);
            if (!count) res = RES_OK;
        }
    }

    deselect();
    return res;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (Stat & STA_PROTECT) return RES_WRPRT;

    if (!(CardType & CT_BLOCK)) sector *= 512;

    DRESULT res = RES_ERROR;

    if (count == 1) {
        if (send_cmd(CMD24, sector) == 0 && xmit_datablock(buff, 0xFE)) res = RES_OK;
    } else {
        if (CardType & CT_SDC) send_cmd(ACMD23, count);

        if (send_cmd(CMD25, sector) == 0) {
            do {
                if (!xmit_datablock(buff, 0xFC)) break;
                buff += 512;
            } while (--count);

            if (!xmit_datablock(0, 0xFD)) count = 1;
            if (!count) res = RES_OK;
        }
    }

    deselect();
    return res;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    DRESULT res = RES_ERROR;

    switch (cmd)
    {
        case CTRL_SYNC:
            if (select()) {
                deselect();
                res = RES_OK;
            }
            break;

        case GET_SECTOR_COUNT:
        {
            BYTE csd[16];
            DWORD csize;

            if (send_cmd(CMD9, 0) == 0 && rcvr_datablock(csd, 16))
            {
                if ((csd[0] >> 6) == 1) { /* CSD v2 */
                    csize = csd[9] + ((WORD)csd[8] << 8)
                          + ((DWORD)(csd[7] & 63) << 16) + 1;
                    *(DWORD*)buff = csize << 10;
                } else { /* CSD v1 */
                    BYTE n = (csd[5] & 15)
                           + ((csd[10] & 128) >> 7)
                           + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6)
                          + ((WORD)csd[7] << 2)
                          + ((WORD)(csd[6] & 3) << 10) + 1;
                    *(DWORD*)buff = csize << (n - 9);
                }
                res = RES_OK;
            }
            deselect();
            break;
        }

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 128;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}

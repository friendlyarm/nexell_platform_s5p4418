/*-----------------------------------------------------------------------------
  파 일 : zb_cpu_s5pv210_regdata.h
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-01-04
  주 의 :

-------------------------------------------------------------------------------*/


// Platform Register Base : A0 0000h ~ A0 1FFFh 
extern reg_t regv_armmp[];

extern reg_t regv_scu[];

extern reg_t regv_ici[];

extern reg_t regv_gtimer[];


extern reg_t regv_ptimer_watchdog[];

extern reg_t regv_interrupt_distibutor[];

extern reg_t regv_epit[];

extern reg_t regv_gpt[];

extern reg_t regv_uart1[];
extern reg_t regv_uart2[];
extern reg_t regv_uart3[];
extern reg_t regv_uart4[];
extern reg_t regv_uart5[];

#if 0
// System Timer  0xE2800000, size=0x100 
extern reg_t regv_rtctimer[];

// System Timer  0xE2500000, size=0x100 
extern reg_t regv_pwmtimer[];

// System Timer  0xE2600000, size=0x100 
extern reg_t regv_systimer[];

// VIC0 0xF2000000, size=0x1000 
extern reg_t regv_vic0[];

// VIC1 0xF2000000, size=0x1000 
extern reg_t regv_vic1[];

// VIC2 0xF2000000, size=0x1000 
extern reg_t regv_vic2[];

// VIC3 0xF2000000, size=0x1000 
extern reg_t regv_vic3[];

// TZICx  0xF2800000, 0xF29000000, 0xF2A00000, 0xF2B00000
extern reg_t regv_tzic0[];
extern reg_t regv_tzic1[];
extern reg_t regv_tzic2[];
extern reg_t regv_tzic3[];

// GPIO  0xE0200000,  size=0x1000
extern reg_t regv_gpio[];

// Clock Gate  0xE0100000, size=0x8000 
extern reg_t regv_sysclk[];

// SROM  0xB0000000, size=0x100 
extern reg_t regv_srom[];

// TSADC  0xE1700000, size=0x2000 
extern reg_t regv_tsadc[];

// OneNAND  0xB0600000, size=0x2000 
extern reg_t regv_onenand[];

// I2C  0xE1800000, 0xFAB00000, 0xE1A00000  size=0x100 
extern reg_t regv_i2c0[];
extern reg_t regv_i2c1[];
extern reg_t regv_i2c2[];

// AC97  0xE2200000, size=0x100 
extern reg_t regv_ac97[];

// UART  0xE2900000,  size=0x1000 
extern reg_t regv_uart[];

// MMC0  0xEB000000,  size=0x100
extern reg_t regv_mmc0[];

// MMC1  0xEB100000,  size=0x100
extern reg_t regv_mmc1[];

// MMC2  0xEB200000,  size=0x100
extern reg_t regv_mmc2[];

// MMC3  0xEB300000,  size=0x100
extern reg_t regv_mmc3[];

// LCD  0xF8000000,  size=0x8000
extern reg_t regv_lcd[];



// Video  0xF9100000,  size=0x400		
extern reg_t regv_video[];

// Mixer  0xF9200000,  size=0x100		
extern reg_t regv_mixer[];

// HDMI_ctrl  0xFA100000,  size=0x100
extern reg_t regv_hdmi_ctrl[];

// HDMI_core  0xFA110000,  size=0x800
extern reg_t regv_hdmi_core[];

// HDMI-spdif  0xFA130000,  size=0x100
extern reg_t regv_hdmi_spdif[];

// HDMI_i2s  0xFA140000,  size=0x100
extern reg_t regv_hdmi_i2s[];

// HDMI_tg  0xFA150000,  size=0x200
extern reg_t regv_hdmi_tg[];

// HDMI_efuse  0xFA160000,  size=0x100
extern reg_t regv_hdmi_efuse[];

// HDMI_cec  0xE1B00000,  size=0x200
extern reg_t regv_hdmi_cec[];

// HDMI_i2c  0xFA900000,  size=0x100
extern reg_t regv_hdmi_i2c[];


#ifdef CONFIG_ANDROID

// SGX_GSR 0x0xF3000000,  size=0x100
extern reg_t regv_sgx_GSR[];

// SGX_GSR 0x0xF3008000,  size=0x100
extern reg_t regv_sgx_HISR[];

// SGX_HISR FGHI_DWENTRY   0xF300C000,  size=0x2000
extern reg_t regv_sgx_FGHI_DWENTRY[];

// SGX_HISR FGHI_VBDATA    0xF300E000,  size=0x1000 
extern reg_t regv_sgx_FGHI_VBDATA[];

// SGX_HISR FGVS_INSTMEM   0xF3010000,  size=0x2000    
extern reg_t regv_sgx_FGVS_INSTMEM[];

// SGX_HISR FGVS_CFLOAT  0xF3014000,  size=0x1000    
extern reg_t regv_sgx_FGVS_CFLOAT[];

// SGX_HISR FGVS_COTHERS  0xF3018000,  size=0xC000    
extern reg_t regv_sgx_FGVS_COTHERS[];

// SGX_HISR FGPE   0xF3030000,  size=0x100    
extern reg_t regv_sgx_FGPE[];

// SGX_HISR FGRA   0xF3038000,  size=0x800    
extern reg_t regv_sgx_FGRA[];

// SGX_HISR FGPS   0xF3040000,  size=0x2000   
extern reg_t regv_sgx_FGPS_INSTMEM[];

// SGX_HISR FGPS_CFLOAT  0xF3044000,  size=0x1000    
extern reg_t regv_sgx_FGPS_CFLOAT[];

// SGX_HISR FGPS_COTHERS  0xF3048000,  size=0x1000    
extern reg_t regv_sgx_FGRA_COTHERS[];

// SGX_HISR FGTU   0xF3060000,  size=0x400 
extern reg_t regv_sgx_FGTU[];

// SGX_HISR FGPF  0xF3070000,  size=0x100    
extern reg_t regv_sgx_FGPF[];

#endif
#endif

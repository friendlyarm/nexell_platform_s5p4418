#ifndef __FAULT_MAIN_H__
#define	__FAULT_MAIN_H__

#ifdef CONFIG_SMP
struct nal_smp {
	u32 nalcode_first_call;
	u32 nalcode_reentry_count[8];
	u32 nalcode_reentry_old; // 1: fault,  2: kernel
	u32	nalcode_debug_state;
	u32	nal_lock;
	u32	nalcode_last_pte[4];
	u32	nal_access_state;

	int   			m2n_max;
	int   			m2n_count;
	int   			m2n_stat;
	m2n_item_t *	m2n_datas;
	int				m2n_dram_size;
	int				m2n_dram_page_max;

	// l2x0 support
	void			*l2x0_base;
	u32				l2x0_way_mask;
	u32				l2x0_size;

	// profile
	int				prof_fault_count;
};

#define	nalcode_first_call		smp_data->nalcode_first_call
#define	nalcode_reentry_count	smp_data->nalcode_reentry_count
#define	nalcode_reentry_old		smp_data->nalcode_reentry_old
#define	nalcode_debug_state		smp_data->nalcode_debug_state
#define	nal_lock				smp_data->nal_lock
#define	nalcode_last_pte		smp_data->nalcode_last_pte
#define	nal_access_state		smp_data->nal_access_state

#define	m2n_max				smp_data->m2n_max
#define	m2n_count			smp_data->m2n_count
#define	m2n_stat			smp_data->m2n_stat
#define	m2n_datas			smp_data->m2n_datas
#define	m2n_dram_size		smp_data->m2n_dram_size
#define	m2n_dram_page_max	smp_data->m2n_dram_page_max

#define	l2x0_base			smp_data->l2x0_base
#define	l2x0_way_mask		smp_data->l2x0_way_mask
#define	l2x0_size			smp_data->l2x0_size

#define	prof_fault_count		smp_data->prof_fault_count
#endif

extern struct nal_smp *smp_data;
void nal_set_smp_data(void);

#define	DEBUG_REENTRANCE
#ifdef DEBUG_REENTRANCE
#define	reentrance_debug_enter	_reentrance_debug_enter
#define	reentrance_debug_exit	_reentrance_debug_exit
#else
static void reentrance_debug_enter(u32 mode, u32 save_lr) {};
static void reentrance_debug_exit(void) {};
#endif

extern unsigned int get_contextidr(void);

struct fault_info {
	u32		vaddr;			// virtual address
	u32		paddr;			// phsical address

	u32		*vpte;			// virtual pte
	u32		*dpte;			// direct mapped pte

	u32		vpte_val;
	u32		dpte_val;
};
#endif // __FAULT_MAIN_H__

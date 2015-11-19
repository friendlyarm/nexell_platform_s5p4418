
#ifdef NAL_FUNC_BEGIN
#undef NAL_FUNC_BEGIN
#undef NAL_FUNC
#undef NAL_FUNC_END
#endif

#define	NAL_CALL(funcname, ...)	nalcode_func((unsigned long)(&((nal_func_t *)0)->nal_##funcname), __VA_ARGS__)

#ifdef HEAD
#define	NAL_FUNC_BEGIN
#define	NAL_FUNC(ret , funcname, ...)	extern	ret nal_##funcname( __VA_ARGS__ );
#define	NAL_FUNC_END
#endif

#ifdef STRUCT
#define	NAL_FUNC_BEGIN					typedef struct {
#define	NAL_FUNC(ret , funcname, ...)	ret (*nal_##funcname)( __VA_ARGS__ );
#define	NAL_FUNC_END					} nal_func_t;
#endif

#ifdef INSTANCE
#define	NAL_FUNC_BEGIN					nal_func_t nal_func = {
#define	NAL_FUNC(ret , funcname, ...)	.nal_##funcname = nal_##funcname,
#define	NAL_FUNC_END					};
#endif

NAL_FUNC_BEGIN
// from nal_m2n.c
NAL_FUNC(void, m2n_open, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, m2n_set_count, u32 offset, u32 used_count, u32 a, u32 b)
NAL_FUNC(void, m2n_set_dev_status, u32 offset, u32 val, u32 a, u32 b)
NAL_FUNC(void, m2n_get_dev_status, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, m2n_wait_lock, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, m2n_set_dram_size, u32 offset, u32 dram_size, u32 a, u32 b)
NAL_FUNC(u32 , m2n_get_count, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(u32 , m2n_get_size, u32 offset, u32 param, u32 param1, u32 param2)
NAL_FUNC(void, m2n_set_mem, u32 offset, u32 mem, u32 phys, u32 b)
NAL_FUNC(u32 , m2n_get_mem, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(u32 , m2n_get_info, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, m2n_clean, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, m2n_add, u32 offset, u32 paddr, u32 info, u32 a)
NAL_FUNC(void, disp_m2n_list, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(void, disp_zba_info, u32 offset, u32 a, u32 b, u32 c)
NAL_FUNC(u32 , __m2n_set_nand_page, u32 offset, u32 item, u32 nand_page, u32 a)
NAL_FUNC(u32 , __m2n_get_nand_page, u32 offset, u32 item, u32 a, u32 b)
NAL_FUNC(void, read_mem_from_paddr, u32 offset, u32 paddr, u32 vaddr, u32 a)
NAL_FUNC(void, debug_set, u32 offset, u32 opt, u32 a, u32 b)
NAL_FUNC(u32 , m2n_get_item_boot_copy, u32 offset, u32 kitem, u32 a, u32 b)
NAL_FUNC(void, m2n_set_page_value, u32 offset, u32 value, u32 a, u32 b)
// if release version return 1
NAL_FUNC(u32 , m2n_get_release_info, u32 offset, u32 a, u32 b, u32 c)

NAL_FUNC(void, m2n_set_mark_protect_bootloader_manage_area		, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_area_of_preloading					, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_boot_copy_area_of_mmu_l1			, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_boot_copy_area_of_mmu_l2			, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_boot_copy_area_of_nalcode			, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_boot_copy_area_of_vector			, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_boot_copy_area_of_video				, u32 offset, u32 paddr, u32 size, u32 a)
NAL_FUNC(void, m2n_set_mark_area_of_work						, u32 offset, u32 paddr, u32 size, u32 a)

NAL_FUNC(void, m2n_mark_fault, u32 offset, u32 pte_paddr, u32 ppte_buf) 
NAL_FUNC(u32 , m2n_build_block_page, u32 offset, u32 start_page, u32 a, u32 b) 
NAL_FUNC(u32 , m2n_get_block_offset, u32 offset, u32 index, u32 a, u32 b) 
NAL_FUNC(u32 , m2n_get_block_phys, u32 offset, u32 index, u32 a, u32 b) 
NAL_FUNC(u32 , m2n_set_debug_checksum, u32 offset, u32 paddr, u32 sum, u32 a) 
NAL_FUNC(void, m2n_blk_test_function, u32 offset, u32 index, u32 a, u32 b) 

NAL_FUNC_END


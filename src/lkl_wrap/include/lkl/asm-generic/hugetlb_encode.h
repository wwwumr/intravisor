#ifndef _LKL_ASM_GENERIC_HUGETLB_ENCODE_H_
#define _LKL_ASM_GENERIC_HUGETLB_ENCODE_H_

/*
 * Several system calls take a flag to request "hugetlb" huge pages.
 * Without further specification, these system calls will use the
 * system's default huge page size.  If a system supports multiple
 * huge page sizes, the desired huge page size can be specified in
 * bits [26:31] of the flag arguments.  The value in these 6 bits
 * will encode the log2 of the huge page size.
 *
 * The following definitions are associated with this huge page size
 * encoding in flag arguments.  System call specific header files
 * that use this encoding should include this file.  They can then
 * provide definitions based on these with their own specific prefix.
 * for example:
 * #define LKL_MAP_HUGE_SHIFT LKL_HUGETLB_FLAG_ENCODE_SHIFT
 */

#define LKL_HUGETLB_FLAG_ENCODE_SHIFT	26
#define LKL_HUGETLB_FLAG_ENCODE_MASK	0x3f

#define LKL_HUGETLB_FLAG_ENCODE_16KB	(14U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_64KB	(16U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_512KB	(19U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_1MB		(20U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_2MB		(21U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_8MB		(23U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_16MB	(24U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_32MB	(25U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_256MB	(28U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_512MB	(29U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_1GB		(30U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_2GB		(31U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)
#define LKL_HUGETLB_FLAG_ENCODE_16GB	(34U << LKL_HUGETLB_FLAG_ENCODE_SHIFT)

#endif /* _LKL_ASM_GENERIC_HUGETLB_ENCODE_H_ */

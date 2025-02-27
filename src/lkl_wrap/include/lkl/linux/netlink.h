/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __LKL__LINUX_NETLINK_H
#define __LKL__LINUX_NETLINK_H

#include <lkl/linux/const.h>
#include <lkl/linux/socket.h> /* for __lkl__kernel_sa_family_t */
#include <lkl/linux/types.h>

#define LKL_NETLINK_ROUTE		0	/* Routing/device hook				*/
#define LKL_NETLINK_UNUSED		1	/* Unused number				*/
#define LKL_NETLINK_USERSOCK	2	/* Reserved for user mode socket protocols 	*/
#define LKL_NETLINK_FIREWALL	3	/* Unused number, formerly ip_queue		*/
#define LKL_NETLINK_SOCK_DIAG	4	/* socket monitoring				*/
#define LKL_NETLINK_NFLOG		5	/* netfilter/iptables ULOG */
#define LKL_NETLINK_XFRM		6	/* ipsec */
#define LKL_NETLINK_SELINUX		7	/* SELinux event notifications */
#define LKL_NETLINK_ISCSI		8	/* Open-iSCSI */
#define LKL_NETLINK_AUDIT		9	/* auditing */
#define LKL_NETLINK_FIB_LOOKUP	10	
#define LKL_NETLINK_CONNECTOR	11
#define LKL_NETLINK_NETFILTER	12	/* netfilter subsystem */
#define LKL_NETLINK_IP6_FW		13
#define LKL_NETLINK_DNRTMSG		14	/* DECnet routing messages (obsolete) */
#define LKL_NETLINK_KOBJECT_UEVENT	15	/* Kernel messages to userspace */
#define LKL_NETLINK_GENERIC		16
/* leave room for NETLINK_DM (DM Events) */
#define LKL_NETLINK_SCSITRANSPORT	18	/* SCSI Transports */
#define LKL_NETLINK_ECRYPTFS	19
#define LKL_NETLINK_RDMA		20
#define LKL_NETLINK_CRYPTO		21	/* Crypto layer */
#define LKL_NETLINK_SMC		22	/* SMC monitoring */

#define LKL_NETLINK_INET_DIAG	LKL_NETLINK_SOCK_DIAG

#define LKL_MAX_LINKS 32		

struct lkl_sockaddr_nl {
	__lkl__kernel_sa_family_t	nl_family;	/* LKL_AF_NETLINK	*/
	unsigned short	nl_pad;		/* zero		*/
	__lkl__u32		nl_pid;		/* port ID	*/
       	__lkl__u32		nl_groups;	/* multicast groups mask */
};

/**
 * struct lkl_nlmsghdr - fixed format metadata header of Netlink messages
 * @nlmsg_len:   Length of message including header
 * @nlmsg_type:  Message content type
 * @nlmsg_flags: Additional flags
 * @nlmsg_seq:   Sequence number
 * @nlmsg_pid:   Sending process port ID
 */
struct lkl_nlmsghdr {
	__lkl__u32		nlmsg_len;
	__lkl__u16		nlmsg_type;
	__lkl__u16		nlmsg_flags;
	__lkl__u32		nlmsg_seq;
	__lkl__u32		nlmsg_pid;
};

/* Flags values */

#define LKL_NLM_F_REQUEST		0x01	/* It is request message. 	*/
#define LKL_NLM_F_MULTI		0x02	/* Multipart message, terminated by LKL_NLMSG_DONE */
#define LKL_NLM_F_ACK		0x04	/* Reply with ack, with zero or error code */
#define LKL_NLM_F_ECHO		0x08	/* Receive resulting notifications */
#define LKL_NLM_F_DUMP_INTR		0x10	/* Dump was inconsistent due to sequence change */
#define LKL_NLM_F_DUMP_FILTERED	0x20	/* Dump was filtered as requested */

/* Modifiers to GET request */
#define LKL_NLM_F_ROOT	0x100	/* specify tree	root	*/
#define LKL_NLM_F_MATCH	0x200	/* return all matching	*/
#define LKL_NLM_F_ATOMIC	0x400	/* atomic GET		*/
#define LKL_NLM_F_DUMP	(LKL_NLM_F_ROOT|LKL_NLM_F_MATCH)

/* Modifiers to NEW request */
#define LKL_NLM_F_REPLACE	0x100	/* Override existing		*/
#define LKL_NLM_F_EXCL	0x200	/* Do not touch, if it exists	*/
#define LKL_NLM_F_CREATE	0x400	/* Create, if it does not exist	*/
#define LKL_NLM_F_APPEND	0x800	/* Add to end of list		*/

/* Modifiers to DELETE request */
#define LKL_NLM_F_NONREC	0x100	/* Do not delete recursively	*/
#define LKL_NLM_F_BULK	0x200	/* Delete multiple objects	*/

/* Flags for ACK message */
#define LKL_NLM_F_CAPPED	0x100	/* request was capped */
#define LKL_NLM_F_ACK_TLVS	0x200	/* extended ACK TVLs were included */

/*
   4.4BSD ADD		LKL_NLM_F_CREATE|LKL_NLM_F_EXCL
   4.4BSD CHANGE	LKL_NLM_F_REPLACE

   True CHANGE		LKL_NLM_F_CREATE|LKL_NLM_F_REPLACE
   Append		LKL_NLM_F_CREATE
   Check		LKL_NLM_F_EXCL
 */

#define LKL_NLMSG_ALIGNTO	4U
#define LKL_NLMSG_ALIGN(len) ( ((len)+LKL_NLMSG_ALIGNTO-1) & ~(LKL_NLMSG_ALIGNTO-1) )
#define LKL_NLMSG_HDRLEN	 ((int) LKL_NLMSG_ALIGN(sizeof(struct lkl_nlmsghdr)))
#define LKL_NLMSG_LENGTH(len) ((len) + LKL_NLMSG_HDRLEN)
#define LKL_NLMSG_SPACE(len) LKL_NLMSG_ALIGN(LKL_NLMSG_LENGTH(len))
#define LKL_NLMSG_DATA(nlh)  ((void *)(((char *)nlh) + LKL_NLMSG_HDRLEN))
#define LKL_NLMSG_NEXT(nlh,len)	 ((len) -= LKL_NLMSG_ALIGN((nlh)->nlmsg_len), \
				  (struct lkl_nlmsghdr *)(((char *)(nlh)) + \
				  LKL_NLMSG_ALIGN((nlh)->nlmsg_len)))
#define LKL_NLMSG_OK(nlh,len) ((len) >= (int)sizeof(struct lkl_nlmsghdr) && \
			   (nlh)->nlmsg_len >= sizeof(struct lkl_nlmsghdr) && \
			   (nlh)->nlmsg_len <= (len))
#define LKL_NLMSG_PAYLOAD(nlh,len) ((nlh)->nlmsg_len - LKL_NLMSG_SPACE((len)))

#define LKL_NLMSG_NOOP		0x1	/* Nothing.		*/
#define LKL_NLMSG_ERROR		0x2	/* Error		*/
#define LKL_NLMSG_DONE		0x3	/* End of a dump	*/
#define LKL_NLMSG_OVERRUN		0x4	/* Data lost		*/

#define LKL_NLMSG_MIN_TYPE		0x10	/* < 0x10: reserved control messages */

struct lkl_nlmsgerr {
	int		error;
	struct lkl_nlmsghdr msg;
	/*
	 * followed by the message contents unless LKL_NETLINK_CAP_ACK was set
	 * or the ACK indicates success (error == 0)
	 * message length is aligned with LKL_NLMSG_ALIGN()
	 */
	/*
	 * followed by TLVs defined in enum lkl_nlmsgerr_attrs
	 * if LKL_NETLINK_EXT_ACK was set
	 */
};

/**
 * enum lkl_nlmsgerr_attrs - nlmsgerr attributes
 * @LKL_NLMSGERR_ATTR_UNUSED: unused
 * @LKL_NLMSGERR_ATTR_MSG: error message string (string)
 * @LKL_NLMSGERR_ATTR_OFFS: offset of the invalid attribute in the original
 *	 message, counting from the beginning of the header (lkl_u32)
 * @LKL_NLMSGERR_ATTR_COOKIE: arbitrary subsystem specific cookie to
 *	be used - in the success case - to identify a created
 *	object or operation or similar (binary)
 * @LKL_NLMSGERR_ATTR_POLICY: policy for a rejected attribute
 * @LKL_NLMSGERR_ATTR_MISS_TYPE: type of a missing required attribute,
 *	%LKL_NLMSGERR_ATTR_MISS_NEST will not be present if the attribute was
 *	missing at the message level
 * @LKL_NLMSGERR_ATTR_MISS_NEST: offset of the nest where attribute was missing
 * @__LKL__NLMSGERR_ATTR_MAX: number of attributes
 * @LKL_NLMSGERR_ATTR_MAX: highest attribute number
 */
enum lkl_nlmsgerr_attrs {
	LKL_NLMSGERR_ATTR_UNUSED,
	LKL_NLMSGERR_ATTR_MSG,
	LKL_NLMSGERR_ATTR_OFFS,
	LKL_NLMSGERR_ATTR_COOKIE,
	LKL_NLMSGERR_ATTR_POLICY,
	LKL_NLMSGERR_ATTR_MISS_TYPE,
	LKL_NLMSGERR_ATTR_MISS_NEST,

	__LKL__NLMSGERR_ATTR_MAX,
	LKL_NLMSGERR_ATTR_MAX = __LKL__NLMSGERR_ATTR_MAX - 1
};

#define LKL_NETLINK_ADD_MEMBERSHIP		1
#define LKL_NETLINK_DROP_MEMBERSHIP		2
#define LKL_NETLINK_PKTINFO			3
#define LKL_NETLINK_BROADCAST_ERROR		4
#define LKL_NETLINK_NO_ENOBUFS		5
#define LKL_NETLINK_RX_RING			6
#define LKL_NETLINK_TX_RING			7
#define LKL_NETLINK_LISTEN_ALL_NSID		8
#define LKL_NETLINK_LIST_MEMBERSHIPS	9
#define LKL_NETLINK_CAP_ACK			10
#define LKL_NETLINK_EXT_ACK			11
#define LKL_NETLINK_GET_STRICT_CHK		12

struct lkl_nl_pktinfo {
	__lkl__u32	group;
};

struct lkl_nl_mmap_req {
	unsigned int	nm_block_size;
	unsigned int	nm_block_nr;
	unsigned int	nm_frame_size;
	unsigned int	nm_frame_nr;
};

struct lkl_nl_mmap_hdr {
	unsigned int	nm_status;
	unsigned int	nm_len;
	__lkl__u32		nm_group;
	/* credentials */
	__lkl__u32		nm_pid;
	__lkl__u32		nm_uid;
	__lkl__u32		nm_gid;
};

enum lkl_nl_mmap_status {
	LKL_NL_MMAP_STATUS_UNUSED,
	LKL_NL_MMAP_STATUS_RESERVED,
	LKL_NL_MMAP_STATUS_VALID,
	LKL_NL_MMAP_STATUS_COPY,
	LKL_NL_MMAP_STATUS_SKIP,
};

#define LKL_NL_MMAP_MSG_ALIGNMENT		LKL_NLMSG_ALIGNTO
#define LKL_NL_MMAP_MSG_ALIGN(sz)		__LKL__ALIGN_KERNEL(sz, LKL_NL_MMAP_MSG_ALIGNMENT)
#define LKL_NL_MMAP_HDRLEN			LKL_NL_MMAP_MSG_ALIGN(sizeof(struct lkl_nl_mmap_hdr))

#define LKL_NET_MAJOR 36		/* Major 36 is reserved for networking 						*/

enum {
	LKL_NETLINK_UNCONNECTED = 0,
	LKL_NETLINK_CONNECTED,
};

/*
 *  <------- LKL_NLA_HDRLEN ------> <-- LKL_NLA_ALIGN(payload)-->
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 * |        Header       | Pad |     Payload       | Pad |
 * |   (struct lkl_nlattr)   | ing |                   | ing |
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 *  <-------------- nlattr->nla_len -------------->
 */

struct lkl_nlattr {
	__lkl__u16           nla_len;
	__lkl__u16           nla_type;
};

/*
 * nla_type (16 bits)
 * +---+---+-------------------------------+
 * | N | O | Attribute Type                |
 * +---+---+-------------------------------+
 * N := Carries nested attributes
 * O := Payload stored in network byte order
 *
 * Note: The N and O flag are mutually exclusive.
 */
#define LKL_NLA_F_NESTED		(1 << 15)
#define LKL_NLA_F_NET_BYTEORDER	(1 << 14)
#define LKL_NLA_TYPE_MASK		~(LKL_NLA_F_NESTED | LKL_NLA_F_NET_BYTEORDER)

#define LKL_NLA_ALIGNTO		4
#define LKL_NLA_ALIGN(len)		(((len) + LKL_NLA_ALIGNTO - 1) & ~(LKL_NLA_ALIGNTO - 1))
#define LKL_NLA_HDRLEN		((int) LKL_NLA_ALIGN(sizeof(struct lkl_nlattr)))

/* Generic 32 bitflags attribute content sent to the kernel.
 *
 * The value is a bitmap that defines the values being set
 * The selector is a bitmask that defines which value is legit
 *
 * Examples:
 *  value = 0x0, and selector = 0x1
 *  implies we are selecting bit 1 and we want to set its value to 0.
 *
 *  value = 0x2, and selector = 0x2
 *  implies we are selecting bit 2 and we want to set its value to 1.
 *
 */
struct lkl_nla_bitfield32 {
	__lkl__u32 value;
	__lkl__u32 selector;
};

/*
 * policy descriptions - it's specific to each family how this is used
 * Normally, it should be retrieved via a dump inside another attribute
 * specifying where it applies.
 */

/**
 * enum lkl_netlink_attribute_type - type of an attribute
 * @LKL_NL_ATTR_TYPE_INVALID: unused
 * @LKL_NL_ATTR_TYPE_FLAG: flag attribute (present/not present)
 * @LKL_NL_ATTR_TYPE_U8: 8-bit unsigned attribute
 * @LKL_NL_ATTR_TYPE_U16: 16-bit unsigned attribute
 * @LKL_NL_ATTR_TYPE_U32: 32-bit unsigned attribute
 * @LKL_NL_ATTR_TYPE_U64: 64-bit unsigned attribute
 * @LKL_NL_ATTR_TYPE_S8: 8-bit signed attribute
 * @LKL_NL_ATTR_TYPE_S16: 16-bit signed attribute
 * @LKL_NL_ATTR_TYPE_S32: 32-bit signed attribute
 * @LKL_NL_ATTR_TYPE_S64: 64-bit signed attribute
 * @LKL_NL_ATTR_TYPE_BINARY: binary data, min/max length may be specified
 * @LKL_NL_ATTR_TYPE_STRING: string, min/max length may be specified
 * @LKL_NL_ATTR_TYPE_NUL_STRING: NUL-terminated string,
 *	min/max length may be specified
 * @LKL_NL_ATTR_TYPE_NESTED: nested, i.e. the content of this attribute
 *	consists of sub-attributes. The nested policy and maxtype
 *	inside may be specified.
 * @LKL_NL_ATTR_TYPE_NESTED_ARRAY: nested array, i.e. the content of this
 *	attribute contains sub-attributes whose type is irrelevant
 *	(just used to separate the array entries) and each such array
 *	entry has attributes again, the policy for those inner ones
 *	and the corresponding maxtype may be specified.
 * @LKL_NL_ATTR_TYPE_BITFIELD32: &struct lkl_nla_bitfield32 attribute
 */
enum lkl_netlink_attribute_type {
	LKL_NL_ATTR_TYPE_INVALID,

	LKL_NL_ATTR_TYPE_FLAG,

	LKL_NL_ATTR_TYPE_U8,
	LKL_NL_ATTR_TYPE_U16,
	LKL_NL_ATTR_TYPE_U32,
	LKL_NL_ATTR_TYPE_U64,

	LKL_NL_ATTR_TYPE_S8,
	LKL_NL_ATTR_TYPE_S16,
	LKL_NL_ATTR_TYPE_S32,
	LKL_NL_ATTR_TYPE_S64,

	LKL_NL_ATTR_TYPE_BINARY,
	LKL_NL_ATTR_TYPE_STRING,
	LKL_NL_ATTR_TYPE_NUL_STRING,

	LKL_NL_ATTR_TYPE_NESTED,
	LKL_NL_ATTR_TYPE_NESTED_ARRAY,

	LKL_NL_ATTR_TYPE_BITFIELD32,
};

/**
 * enum lkl_netlink_policy_type_attr - policy type attributes
 * @LKL_NL_POLICY_TYPE_ATTR_UNSPEC: unused
 * @LKL_NL_POLICY_TYPE_ATTR_TYPE: type of the attribute,
 *	&enum lkl_netlink_attribute_type (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_MIN_VALUE_S: minimum value for signed
 *	integers (S64)
 * @LKL_NL_POLICY_TYPE_ATTR_MAX_VALUE_S: maximum value for signed
 *	integers (S64)
 * @LKL_NL_POLICY_TYPE_ATTR_MIN_VALUE_U: minimum value for unsigned
 *	integers (U64)
 * @LKL_NL_POLICY_TYPE_ATTR_MAX_VALUE_U: maximum value for unsigned
 *	integers (U64)
 * @LKL_NL_POLICY_TYPE_ATTR_MIN_LENGTH: minimum length for binary
 *	attributes, no minimum if not given (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_MAX_LENGTH: maximum length for binary
 *	attributes, no maximum if not given (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_POLICY_IDX: sub policy for nested and
 *	nested array types (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_POLICY_MAXTYPE: maximum sub policy
 *	attribute for nested and nested array types, this can
 *	in theory be < the size of the policy pointed to by
 *	the index, if limited inside the nesting (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_BITFIELD32_MASK: valid mask for the
 *	bitfield32 type (U32)
 * @LKL_NL_POLICY_TYPE_ATTR_MASK: mask of valid bits for unsigned integers (U64)
 * @LKL_NL_POLICY_TYPE_ATTR_PAD: pad attribute for 64-bit alignment
 *
 * @__LKL__NL_POLICY_TYPE_ATTR_MAX: number of attributes
 * @LKL_NL_POLICY_TYPE_ATTR_MAX: highest attribute number
 */
enum lkl_netlink_policy_type_attr {
	LKL_NL_POLICY_TYPE_ATTR_UNSPEC,
	LKL_NL_POLICY_TYPE_ATTR_TYPE,
	LKL_NL_POLICY_TYPE_ATTR_MIN_VALUE_S,
	LKL_NL_POLICY_TYPE_ATTR_MAX_VALUE_S,
	LKL_NL_POLICY_TYPE_ATTR_MIN_VALUE_U,
	LKL_NL_POLICY_TYPE_ATTR_MAX_VALUE_U,
	LKL_NL_POLICY_TYPE_ATTR_MIN_LENGTH,
	LKL_NL_POLICY_TYPE_ATTR_MAX_LENGTH,
	LKL_NL_POLICY_TYPE_ATTR_POLICY_IDX,
	LKL_NL_POLICY_TYPE_ATTR_POLICY_MAXTYPE,
	LKL_NL_POLICY_TYPE_ATTR_BITFIELD32_MASK,
	LKL_NL_POLICY_TYPE_ATTR_PAD,
	LKL_NL_POLICY_TYPE_ATTR_MASK,

	/* keep last */
	__LKL__NL_POLICY_TYPE_ATTR_MAX,
	LKL_NL_POLICY_TYPE_ATTR_MAX = __LKL__NL_POLICY_TYPE_ATTR_MAX - 1
};

#endif /* __LKL__LINUX_NETLINK_H */

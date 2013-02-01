#include "../src/hammer.h"

enum DNSTokenType_ {
  TT_dns_message_t = TT_USER,
  TT_dns_header_t,
  TT_dns_label_t,
  TT_dns_qname_t,
  TT_dns_question_t,
  TT_dns_rr_t,
  TT_dns_rr_txt_t,
  TT_dns_rr_hinfo_t,
  TT_dns_rr_minfo_t,
  TT_dns_rr_mx_t,
  TT_dns_rr_soa_t,
  TT_dns_rr_wks_t,
  TT_dns_rr_null_t,
  TT_dns_domain_t,
  TT_dns_cstr_t
};

typedef char *dns_domain_t;
typedef uint8_t *dns_cstr_t;

typedef struct dns_header {
  uint16_t id;
  bool qr, aa, tc, rd, ra;
  char opcode, rcode;
  size_t question_count;
  size_t answer_count;
  size_t authority_count;
  size_t additional_count;
} dns_header_t;

typedef struct dns_label {
  size_t len;
  uint8_t *label;
} dns_label_t;

typedef struct dns_qname {
  size_t qlen;
  dns_label_t *labels;
} dns_qname_t;

typedef struct dns_question {
  dns_qname_t qname;
  uint16_t qtype;
  uint16_t qclass;
} dns_question_t;

typedef struct {
  dns_cstr_t cpu;
  dns_cstr_t os;
} dns_rr_hinfo_t;

typedef struct {
  char* rmailbx;
  char* emailbx;
} dns_rr_minfo_t;

typedef struct {
  uint16_t preference;
  char* exchange;
} dns_rr_mx_t;

typedef struct {
  char* mname;
  char* rname;
  uint32_t serial;
  uint32_t refresh;
  uint32_t retry;
  uint32_t expire;
  uint32_t minimum;
} dns_rr_soa_t;

typedef struct {
  size_t count;
  uint8_t** txt_data;
} dns_rr_txt_t;

typedef struct {
  uint32_t address;
  uint8_t protocol;
  size_t len;
  uint8_t* bit_map;
} dns_rr_wks_t;

typedef uint8_t *dns_rr_null_t;

typedef struct dns_rr {
  char* name;
  uint16_t type;
  uint16_t class;
  uint32_t ttl; // cmos is also acceptable.
  uint16_t rdlength;
  union {
    uint32_t       a;
    char*          ns;
    char*          md;
    char*          mf;
    char*          cname;
    dns_rr_soa_t   soa;
    char*          mb;
    char*          mg;
    char*          mr;
    dns_rr_null_t  null;
    dns_rr_wks_t   wks;
    char*          ptr;
    dns_rr_hinfo_t hinfo;
    dns_rr_minfo_t minfo;
    dns_rr_mx_t    mx;
    dns_rr_txt_t   txt;
  };
} dns_rr_t;

typedef struct dns_message {
  dns_header_t header;
  dns_question_t *questions;
  dns_rr_t *answers;
  dns_rr_t *authority;
  dns_rr_t *additional;
} dns_message_t;

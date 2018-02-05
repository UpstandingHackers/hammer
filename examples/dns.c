#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#include <string.h>
#include "../src/hammer.h"
#include "dns_common.h"
#include "dns.h"
#include "rr.h"

#define false 0
#define true 1


///
// Validations
///

bool validate_hdzero(HParseResult *p, void* user_data) {
  if (TT_UINT != p->ast->token_type)
    return false;
  return (0 == p->ast->uint);
}

/**
 * Every DNS message should have QDCOUNT entries in the question
 * section, and ANCOUNT+NSCOUNT+ARCOUNT resource records.
 */
bool validate_message(HParseResult *p, void* user_data) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;

  dns_header_t *header = H_FIELD(dns_header_t, 0);
  size_t qd = header->question_count;
  size_t an = header->answer_count;
  size_t ns = header->authority_count;
  size_t ar = header->additional_count;

  if (H_FIELD_SEQ(1)->used != qd)
    return false;
  if (an+ns+ar != H_FIELD_SEQ(2)->used)
    return false;

  return true;
}


///
// Semantic Actions
///

// Helper: Parse and pack the RDATA field of a Resource Record.
void set_rdata(struct dns_rr *rr, HCountedArray *rdata) {
  uint8_t *data = h_arena_malloc(rdata->arena, sizeof(uint8_t)*rdata->used);
  for (size_t i=0; i<rdata->used; ++i)
    data[i] = H_CAST_UINT(rdata->elements[i]);

  // Parse RDATA if possible.
  const HParseResult *p = NULL;
  const HParser *parser = init_rdata(rr->type);
  if (parser)
    p = h_parse(parser, (const uint8_t*)data, rdata->used);

  // If the RR doesn't parse, set its type to 0.
  if (!p) 
    rr->type = 0;

  // Pack the parsed rdata into rr.
  switch(rr->type) {
  case 1:  rr->a     = H_CAST_UINT(p->ast);             break;
  case 2:  rr->ns    = *H_CAST(dns_domain_t,   p->ast); break;
  case 3:  rr->md    = *H_CAST(dns_domain_t,   p->ast); break;
  case 4:  rr->md    = *H_CAST(dns_domain_t,   p->ast); break;
  case 5:  rr->cname = *H_CAST(dns_domain_t,   p->ast); break;
  case 6:  rr->soa   = *H_CAST(dns_rr_soa_t,   p->ast); break;
  case 7:  rr->mb    = *H_CAST(dns_domain_t,   p->ast); break;
  case 8:  rr->mg    = *H_CAST(dns_domain_t,   p->ast); break;
  case 9:  rr->mr    = *H_CAST(dns_domain_t,   p->ast); break;
  case 10: rr->null  = *H_CAST(dns_rr_null_t,  p->ast); break;
  case 11: rr->wks   = *H_CAST(dns_rr_wks_t,   p->ast); break;
  case 12: rr->ptr   = *H_CAST(dns_domain_t,   p->ast); break;
  case 13: rr->hinfo = *H_CAST(dns_rr_hinfo_t, p->ast); break;
  case 14: rr->minfo = *H_CAST(dns_rr_minfo_t, p->ast); break;
  case 15: rr->mx    = *H_CAST(dns_rr_mx_t,    p->ast); break;
  case 16: rr->txt   = *H_CAST(dns_rr_txt_t,   p->ast); break;
  default:                                              break;
  }
}

HParsedToken* act_header(const HParseResult *p, void* user_data) {
  HParsedToken **fields = h_seq_elements(p->ast);
  dns_header_t header_ = {
    .id     = H_CAST_UINT(fields[0]),
    .qr     = H_CAST_UINT(fields[1]),
    .opcode = H_CAST_UINT(fields[2]),
    .aa     = H_CAST_UINT(fields[3]),
    .tc     = H_CAST_UINT(fields[4]),
    .rd     = H_CAST_UINT(fields[5]),
    .ra     = H_CAST_UINT(fields[6]),
    .rcode  = H_CAST_UINT(fields[7]),
    .question_count   = H_CAST_UINT(fields[8]),
    .answer_count     = H_CAST_UINT(fields[9]),
    .authority_count  = H_CAST_UINT(fields[10]),
    .additional_count = H_CAST_UINT(fields[11])
  };

  dns_header_t *header = H_ALLOC(dns_header_t);
  *header = header_;

  return H_MAKE(dns_header_t, header);
}

HParsedToken* act_label(const HParseResult *p, void* user_data) {
  dns_label_t *r = H_ALLOC(dns_label_t);

  r->len = h_seq_len(p->ast);
  r->label = h_arena_malloc(p->arena, r->len + 1);
  for (size_t i=0; i<r->len; ++i)
    r->label[i] = H_FIELD_UINT(i);
  r->label[r->len] = 0;

  return H_MAKE(dns_label_t, r);
}

HParsedToken* act_rr(const HParseResult *p, void* user_data) {
  dns_rr_t *rr = H_ALLOC(dns_rr_t);

  rr->name     = *H_FIELD(dns_domain_t, 0);
  rr->type     = H_FIELD_UINT(1);
  rr->class    = H_FIELD_UINT(2);
  rr->ttl      = H_FIELD_UINT(3);
  rr->rdlength = H_FIELD_SEQ(4)->used;

  // Parse and pack RDATA.
  set_rdata(rr, H_FIELD_SEQ(4));

  return H_MAKE(dns_rr_t, rr);
}

HParsedToken* act_question(const HParseResult *p, void* user_data) {
  dns_question_t *q = H_ALLOC(dns_question_t);
  HParsedToken **fields = h_seq_elements(p->ast);

  // QNAME is a sequence of labels. Pack them into an array.
  q->qname.qlen   = h_seq_len(fields[0]);
  q->qname.labels = h_arena_malloc(p->arena, sizeof(dns_label_t)*q->qname.qlen);
  for(size_t i=0; i<q->qname.qlen; i++) {
    q->qname.labels[i] = *H_INDEX(dns_label_t, fields[0], i);
  }

  q->qtype  = H_CAST_UINT(fields[1]);
  q->qclass = H_CAST_UINT(fields[2]);

  return H_MAKE(dns_question_t, q);
}

HParsedToken* act_message(const HParseResult *p, void* user_data) {
  h_pprint(stdout, p->ast, 0, 2);
  dns_message_t *msg = H_ALLOC(dns_message_t);

  // Copy header into message struct.
  dns_header_t *header = H_FIELD(dns_header_t, 0);
  msg->header = *header;

  // Copy questions into message struct.
  HParsedToken *qs = h_seq_index(p->ast, 1);
  struct dns_question *questions = h_arena_malloc(p->arena,
						  sizeof(struct dns_question)*(header->question_count));
  for (size_t i=0; i<header->question_count; ++i) {
    questions[i] = *H_INDEX(dns_question_t, qs, i);
  }
  msg->questions = questions;

  // Copy answer RRs into message struct.
  HParsedToken *rrs = h_seq_index(p->ast, 2);
  struct dns_rr *answers = h_arena_malloc(p->arena,
					  sizeof(struct dns_rr)*(header->answer_count));
  for (size_t i=0; i<header->answer_count; ++i) {
    answers[i] = *H_INDEX(dns_rr_t, rrs, i);
  }
  msg->answers = answers;

  // Copy authority RRs into message struct.
  struct dns_rr *authority = h_arena_malloc(p->arena,
					    sizeof(struct dns_rr)*(header->authority_count));
  for (size_t i=0, j=header->answer_count; i<header->authority_count; ++i, ++j) {
    authority[i] = *H_INDEX(dns_rr_t, rrs, j);
  }
  msg->authority = authority;

  // Copy additional RRs into message struct.
  struct dns_rr *additional = h_arena_malloc(p->arena,
					     sizeof(struct dns_rr)*(header->additional_count));
  for (size_t i=0, j=header->answer_count+header->authority_count; i<header->additional_count; ++i, ++j) {
    additional[i] = *H_INDEX(dns_rr_t, rrs, j);
  }
  msg->additional = additional;

  return H_MAKE(dns_message_t, msg);
}

#define act_hdzero h_act_ignore
#define act_qname  act_index0


///
// Grammar
///

const HParser* init_parser() {
  static const HParser *ret = NULL;
  if (ret)
    return ret;

  H_RULE  (domain,   init_domain());
  H_AVRULE(hdzero,   h_bits(3, false));
  H_ARULE (header,   h_sequence(h_bits(16, false), // ID
				h_bits(1, false),  // QR
				h_bits(4, false),  // opcode
				h_bits(1, false),  // AA
				h_bits(1, false),  // TC
				h_bits(1, false),  // RD
				h_bits(1, false),  // RA
				hdzero,            // Z
				h_bits(4, false),  // RCODE
				h_uint16(),        // QDCOUNT
				h_uint16(),        // ANCOUNT
				h_uint16(),        // NSCOUNT
				h_uint16(),        // ARCOUNT
				NULL));
  H_RULE  (type,     h_int_range(h_uint16(), 1, 16));
  H_RULE  (qtype,    h_choice(type, 
			      h_int_range(h_uint16(), 252, 255),
			      NULL));
  H_RULE  (class,    h_int_range(h_uint16(), 1, 4));
  H_RULE  (qclass,   h_choice(class,
			      h_int_range(h_uint16(), 255, 255),
			      NULL));
  H_RULE  (len,      h_int_range(h_uint8(), 1, 255));
  H_ARULE (label,    h_length_value(len, h_uint8())); 
  H_ARULE (qname,    h_sequence(h_many1(label),
				h_ch('\x00'),
				NULL));
  H_ARULE (question, h_sequence(qname, qtype, qclass, NULL));
  H_RULE  (rdata,    h_length_value(h_uint16(), h_uint8()));
  H_ARULE (rr,       h_sequence(domain,            // NAME
				type,              // TYPE
				class,             // CLASS
				h_uint32(),        // TTL
				rdata,             // RDLENGTH+RDATA
				NULL));
  H_AVRULE(message,  h_sequence(header,
				h_many(question),
				h_many(rr),
				h_end_p(),
				NULL));

  ret = message;
  return ret;
}


///
// Main Program for a Dummy DNS Server
///

int start_listening() {
  // return: fd
  int sock;
  struct sockaddr_in addr;

  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    err(1, "Failed to open listning socket");
  addr.sin_family = AF_INET;
  addr.sin_port = htons(53);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    err(1, "Bind failed");
  return sock;
}

const int TYPE_MAX = 16;
static const char* TYPE_STR[17] = {
  "nil", "A", "NS", "MD",
  "MF", "CNAME", "SOA", "MB",
  "MG", "MR", "NULL", "WKS",
  "PTR", "HINFO", "MINFO", "MX",
  "TXT"
};

const int CLASS_MAX = 4;
const char* CLASS_STR[5] = {
  "nil", "IN", "CS", "CH", "HS"
};


void format_qname(struct dns_qname *name, uint8_t **dest) {
  uint8_t *rp = *dest;
  for (size_t j = 0; j < name->qlen; j++) {
    *rp++ = name->labels[j].len;
    for (size_t k = 0; k < name->labels[j].len; k++)
      *rp++ = name->labels[j].label[k];
  }
  *rp++ = 0;
  *dest = rp;
}
      

int main(int argc, char** argv) {
  const HParser *parser = init_parser();
    
  
  // set up a listening socket...
  int sock = start_listening();

  uint8_t packet[8192]; // static buffer for simplicity
  ssize_t packet_size;
  struct sockaddr_in remote;
  socklen_t remote_len;

  
  while (1) {
    remote_len = sizeof(remote);
    packet_size = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr*)&remote, &remote_len);
    // dump the packet...
    for (int i = 0; i < packet_size; i++)
      printf(".%02hhx", packet[i]);
    
    printf("\n");

    HParseResult *content = h_parse(parser, packet, packet_size);
    if (!content) {
      printf("Invalid packet; ignoring\n");
      continue;
    }
    dns_message_t *message = content->ast->user;
    (void)message;
    for (size_t i = 0; i < message->header.question_count; i++) {
      struct dns_question *question = &message->questions[i];
      printf("Recieved %s %s request for ", CLASS_STR[question->qclass], TYPE_STR[question->qtype]);
      for (size_t j = 0; j < question->qname.qlen; j++)
	printf("%s.", question->qname.labels[j].label);
      printf("\n");
      
    }
    printf("%p\n", content);





    
    // Not much time to actually implement the DNS server for the talk, so here's something quick and dirty. 
    // Traditional response for this time of year...
    uint8_t response_buf[4096];
    uint8_t *rp = response_buf;
    // write out header...
    *rp++ = message->header.id >> 8;
    *rp++ = message->header.id & 0xff;
    *rp++ = 0x80 | (message->header.opcode << 3) | message->header.rd;
    *rp++ = 0x0; // change to 0 for no error...
    *rp++ = 0; *rp++ = 1; // QDCOUNT
    *rp++ = 0; *rp++ = 1; // ANCOUNT
    *rp++ = 0; *rp++ = 0; // NSCOUNT
    *rp++ = 0; *rp++ = 0; // ARCOUNT
    // encode the first question...
    {
      struct dns_question *question = &message->questions[0];
      format_qname(&question->qname, &rp);
      *rp++ = (question->qtype >> 8) & 0xff;
      *rp++ = (question->qtype     ) & 0xff;
      *rp++ = (question->qclass >> 8) & 0xff;
      *rp++ = (question->qclass     ) & 0xff;

      // it's a cname...
      format_qname(&question->qname, &rp);
      *rp++ = 0; *rp++ = 5;
      *rp++ = (question->qclass >> 8) & 0xff;
      *rp++ = (question->qclass     ) & 0xff;
      *rp++ = 0; *rp++ = 0; *rp++ = 0; *rp++ = 0; // TTL.
      //const char cname_rd[14] = "\x09spargelze\x02it";
      *rp++ = 0; *rp++ = 14;
      memcpy(rp, "\x09spargelze\x02it", 14);
      rp += 14;
    }
    // send response.
    sendto(sock, response_buf, (rp - response_buf), 0, (struct sockaddr*)&remote, remote_len);
  }
  return 0;
}



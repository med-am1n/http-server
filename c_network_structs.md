# C Socket API — Networking Structures

> These structs are defined in system headers — do **not** redefine them in real projects.

**Required headers:**

| Header | Provides |
|---|---|
| `<sys/socket.h>` | `sockaddr`, `sockaddr_storage` |
| `<netinet/in.h>` | `sockaddr_in`, `sockaddr_in6`, `in_addr`, `in6_addr` |
| `<arpa/inet.h>` | Address conversion utilities |
| `<netdb.h>` | `addrinfo`, `getaddrinfo()` |

---

## 1. Generic Structures

### `sockaddr` — Base Socket Address

The base type accepted by all socket functions. Never used directly — always cast from a concrete type.

```c
struct sockaddr {
    unsigned short sa_family;   // address family (AF_INET, AF_INET6)
    char           sa_data[14]; // protocol-specific address data
};
```

### `sockaddr_storage` — Protocol-Agnostic Container

Use this when the address family is not known ahead of time. Large enough to hold both IPv4 and IPv6 addresses.

```c
struct sockaddr_storage {
    sa_family_t ss_family;               // address family
    char        __ss_pad1[_SS_PAD1SIZE]; // padding
    int64_t     __ss_align;              // alignment
    char        __ss_pad2[_SS_PAD2SIZE]; // padding
};
```

---

## 2. IPv4 Structures

### `sockaddr_in` — IPv4 Socket Address

```c
struct sockaddr_in {
    short int           sin_family;  // AF_INET
    unsigned short int  sin_port;    // port number (network byte order)
    struct in_addr      sin_addr;    // IPv4 address
    unsigned char       sin_zero[8]; // padding (zeroed out)
};
```

### `in_addr` — IPv4 Address

Embedded inside `sockaddr_in`.

```c
struct in_addr {
    uint32_t s_addr; // 32-bit IPv4 address (network byte order)
};
```

---

## 3. IPv6 Structures

### `sockaddr_in6` — IPv6 Socket Address

```c
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // AF_INET6
    u_int16_t       sin6_port;     // port number
    u_int32_t       sin6_flowinfo; // flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // scope ID
};
```

### `in6_addr` — IPv6 Address

Embedded inside `sockaddr_in6`.

```c
struct in6_addr {
    unsigned char s6_addr[16]; // 128-bit IPv6 address
};
```

---

## 4. DNS Resolution

### `addrinfo` — Protocol-Independent Address Info

Used with `getaddrinfo()` for hostname resolution. Returns a linked list of results.

```c
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, or AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM or SOCK_DGRAM
    int              ai_protocol;  // 0 = auto-select
    size_t           ai_addrlen;   // length of ai_addr
    struct sockaddr *ai_addr;      // pointer to sockaddr_in or sockaddr_in6
    char            *ai_canonname; // canonical hostname
    struct addrinfo *ai_next;      // next node in linked list
};
```

---

## 5. Type Casting

All socket API calls accept `struct sockaddr *`, so concrete types must be cast:

```c
// IPv4
bind(sockfd, (struct sockaddr *)&ipv4_addr, sizeof(ipv4_addr));

// IPv6
bind(sockfd, (struct sockaddr *)&ipv6_addr, sizeof(ipv6_addr));
```

The `sa_family` field tells the OS which concrete type is actually being pointed to.

---

## 6. Address Conversion

Convert between human-readable IP strings and binary network form using `inet_pton` / `inet_ntop` (from `<arpa/inet.h>`).

### `inet_pton` — Presentation to Network

Parses a dotted-decimal (IPv4) or colon-hex (IPv6) string into binary form, ready to be stored in a socket address struct.

```c
struct sockaddr_in  sa;  // IPv4
struct sockaddr_in6 sa6; // IPv6

inet_pton(AF_INET,  "10.12.110.57",           &(sa.sin_addr));
inet_pton(AF_INET6, "2001:db8:63b3:1::3490",  &(sa6.sin6_addr));
```

### `inet_ntop` — Network to Presentation

The inverse: converts binary address data back into a human-readable string.

```c
// IPv4
char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
struct sockaddr_in sa;       // pretend this is loaded with something

inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
printf("The IPv4 address is: %s\n", ip4);

// IPv6
char ip6[INET6_ADDRSTRLEN]; // space to hold the IPv6 string
struct sockaddr_in6 sa6;    // pretend this is loaded with something

inet_ntop(AF_INET6, &(sa6.sin6_addr), ip6, INET6_ADDRSTRLEN);
printf("The IPv6 address is: %s\n", ip6);
```

| Constant | Value | Purpose |
|---|---|---|
| `INET_ADDRSTRLEN` | 16 | Buffer size for an IPv4 string |
| `INET6_ADDRSTRLEN` | 46 | Buffer size for an IPv6 string |

> These replace the older `inet_addr()` / `inet_ntoa()` — prefer `pton`/`ntop` in all new code as they handle both IPv4 and IPv6.

---

## Best Practices

- **Never redefine** these structs — always include the appropriate system headers.
- **Prefer `getaddrinfo()`** over manual struct setup for DNS and address handling.
- **Use `sockaddr_storage`** when the address family is determined at runtime.
- **Zero-initialize** structs before use: `memset(&addr, 0, sizeof(addr));`
- **Network byte order** — always convert ports with `htons()` and addresses with `inet_pton()`.

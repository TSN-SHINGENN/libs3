#ifndef INC_MPXX_CTYPE_H
#define INC_MPXX_CTYPE_H

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

int mpxx_ascii_isprint(const int c);
int mpxx_ascii_isspace(const int c);
int mpxx_ascii_isdigit(const int c);
int mpxx_ascii_isalpha(const int c);
int mpxx_ascii_islower(const int c);
int mpxx_ascii_isupper(const int c);

int mpxx_ascii_toupper(const int c);
int mpxx_ascii_tolower(const int c);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_CTYPE_H */

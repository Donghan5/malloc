/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_helpers.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int starts_with(const char *text, const char *prefix);
char *find_text(char *text, const char *needle);
int parse_number(const char **cursor, unsigned int base, uintptr_t *value);
int pattern(const unsigned char *p, size_t n, unsigned char value);
t_heap *owner(void *p);


/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bonus_helpers.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int capture_begin(int *saved);
int capture_end(int fd, int saved, char *output, size_t capacity);
int bonus_run(int argc, char **argv, const char *const *names,
    int (*const *cases)(void), size_t count);
int bonus_valid_state(void);

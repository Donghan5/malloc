/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   all_cases.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

pid_t waitpid(pid_t pid, int *status, int options);
int run_isolated(char *const argv[], int enabled);
int suite_count(const char *binary, size_t *count);

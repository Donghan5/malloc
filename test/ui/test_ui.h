/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_ui.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

void test_ui_start(const char *name);
void test_ui_result(const char *name, int success);
void test_ui_heading(const char *suite);
void test_ui_summary(const char *suite, unsigned int passed, unsigned int total);

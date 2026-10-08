/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bonus_edge_cases.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int edge_m2_split_boundary(void);
int edge_m2_shrink_with_live_neighbors(void);
int edge_m2_reuse_after_merge(void);
int edge_m3_runtime_scribble_toggle(void);
int edge_m3_realloc_null_scribble(void);
int edge_m3_freed_dump_exclusion(void);
int edge_m4_early_return_unlock(void);
int edge_m4_concurrent_first_use(void);
int edge_m4_realloc_failure_unlock(void);
int edge_m5_small_heap_reuse(void);
int edge_m5_large_reclamation(void);
int edge_m5_recovery_after_mapping_failure(void);

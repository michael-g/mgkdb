/* This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
 *
 * The Library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * The Library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License along with The
 * Library. If not, see https://www.gnu.org/licenses/agpl.txt.
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#define KXVER 3
#include "k.h"

K hex_snoop(K obj)
{
  printf("\nAddress %p\n", &*obj);
  printf("obj->m %02x\n", 0xFF & obj->m);
  printf("obj->a %02x\n", 0xFF & obj->a);
  printf("obj->t %hhi 0x%02x\n", obj->t, 0xff & obj->t);
  printf("obj->u %hhi\n", obj->u);
  printf("obj->r %i\n", obj->r);
  printf("obj->g 0x%02x\n", 0xFF & obj->g);
  printf("obj->h %hi\n", obj->h);
  printf("obj->i %i\n", obj->i);
  printf("obj->j %lli\n", obj->j);
  printf("obj->k %p\n", obj->k);
  printf("obj->n %lli\n", obj->n);

  if (0 == obj->t) {
    printf("Skipping atom type\n");
  }
  else if (obj->t > 0 && obj->t < 20) {
    printf("Skipping vector type\n");
    for (J i = 0 ; i < obj->n ; i++) {
    }
  }
  // else if (103 == obj->t) { // 0x67 
  //   for (J i = 0 ; i < obj->n ; i++) {
  //       printf("%lli: %02x\n", i, 0xFF & ((int8_t*)obj->G0)[i]);
  //   }
  // }
  else if (obj->t >= 0x6a && obj->t <= 0x6f) { // 0x6f
    return hex_snoop(obj->k);
  }
  else {
    uint64_t src_adr = 0xFFFFFFFFFFFFF000L & (uint64_t)&*obj;
    uint64_t ptr_adr = 0xFFFFFFFFFFFFF000L & (uint64_t)obj->k;

    if (src_adr == ptr_adr) {
      return hex_snoop((K)obj->k);
    }
    else {
      printf("\nNo special handling for type %hhi\n", obj->t);
    }
  }

  return kb(1);
}

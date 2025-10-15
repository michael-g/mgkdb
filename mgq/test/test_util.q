/ This file is part of the Mg kdb+/mgq Library (hereinafter "The Library").

/ The Library is free software: you can redistribute it and/or modify it under
/ the terms of the GNU Affero Public License as published by the Free Software
/ Foundation, either version 3 of the License, or (at your option) any later
/ version.

/ The Library is distributed in the hope that it will be useful, but WITHOUT ANY
/ WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/ PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

/ You should have received a copy of the GNU Affero Public License along with The
/ Library. If not, see https://www.gnu.org/licenses/agpl.txt.

.utl.tst.arity:{
  .mok.ast.eq[1] .utl.arity .utl.arity
 ;.mok.ast.eq[5] .utl.arity .mok.register
 ;.mok.ast.eq[3] .utl.arity .mok.register[1;;2]
 ;.mok.ast.eq[3] .utl.arity .mok.register[`;;`]
 ;.mok.ast.eq[4] .utl.arity (`.mok.register;;;;4;)
 ;.mok.ast.eq[3] .utl.arity (`.mok.register;1;;;4;)
 ;
 }

.utl.tst.zpcNotifiesCallbackAndRemovesEntries:{
  // Test that the handler for .z.pc:
  // . removes entries from .utl.conns
  // . invokes callbacks registered against the FD argument before 
  .utl.init[]                                                                     // set up the tables .conns and .cbks
 ;.utl.tst.rgs:()                                                                 // initialise an argument catcher
 ;fns:({[H] .utl.tst.rgs,:enlist(0;H)}                                            // create a list of distinguishable callbacks to insert into .cbks
      ;{[H] .utl.tst.rgs,:enlist(1;H)}                                            // 
      ;{[H] .mok.ast.fail "Bad invocation: zpo"}                                  // 
      ;{[H] .mok.ast.fail "Bad invocation: FD 4"})                                // 
 ;.utl.zpcs:enlist {[H] .utl.tst.rgs,:enlist(2;H)}                                // register a single generalist on-close handler
 ;`.utl.cbks insert (3 3 3 4i;`zpc`zpc`zpo`zpc;fns)                               // register the callbacks; two should be called, two should not
 ;`.utl.conns upsert flip (3 4i;``;``;"**")                                       // register two connections with .conns, one to be removed
 ;.utl.zpc 3i                                                                     // call the .z.pc handler
 ;.mok.ast.eq[1] exec count i from .utl.cbks where not null fd                    // assert that the FD 4 callback remains
 ;.mok.ast.is[0N 4i] .utl.cbks`fd                                                 // assert the remaining callbacks are the sentinel and 4i values
 ;.mok.ast.is[0N 4i] (key .utl.conns)`fd                                          // assert the remaining entries in .conns are the sentinel and 4i values
 ;.mok.ast.eq[3] count .utl.tst.rgs                                               // assert that both 3i:zpc handlers were invoked
 ;.mok.ast.eq[1] count where (0;3i)~/: .utl.tst.rgs                               // assert that handler "0" was called
 ;.mok.ast.eq[1] count where (1;3i)~/: .utl.tst.rgs                               // assert that handler "1" was called, order is deliberately not asserted
 ;.mok.ast.eq[1] count where (2;3i)~/: .utl.tst.rgs                               // assert that the .utl.zpcs general callback was invoked
 }

.utl.tst.onZpcCbkErr:{
  // essentially we're testing that the error-handler itself doesn't blow-up
  // and make things worse
  .Q.trp[{'"fail"};3i;.utl.onZpcCbkErr 3i]                                        // invoke the error-handler via .Q.trp
 ;.mok.ast.eq[1] count exec count i from .mok.logged where not null name          // assert the message was logged
 ;msg:exec first arg from .mok.logged where not null name                         // retrieve the message what was logged
 ;.mok.ast.eq[6] count msg                                                        // assert its rough structure
 ;.mok.ast.is[(3i;"fail")] msg 1 3                                                // assert that elements [1] and [3] occur as expected
 }

.mok.test[`util.q;`.utl];

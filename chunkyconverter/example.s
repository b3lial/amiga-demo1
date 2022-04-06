  xdef      _testFunc
  xdef      _addFunc

_testFunc:
  move.w    #42,d0
  rts

_addFunc:
  move.l    4(sp),d0
  add.l     8(sp),d0
  rts
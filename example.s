  xdef      _testFunc
  xdef      _addFunc

_testFunc:
  move.w    #42,d0
  rts

_addFunc:
  move.w    a0,d0
  add.w     a1,d0
  rts
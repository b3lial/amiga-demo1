  xdef      _testFunc

init:
  jsr       _testFunc
  rts

_testFunc:
  move.w    #0000,d0
  rts
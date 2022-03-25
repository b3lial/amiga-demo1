  xdef      _testFunc

init:
  jsr       _testFunc
  rts

_testFunc:
  move.w    #42,d0
  rts
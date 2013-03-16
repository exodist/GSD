set foldmethod=syntax
set nowrap

augroup type
  autocmd BufNewFile,BufRead *.c set foldmethod=syntax
  autocmd BufNewFile,BufRead *.t set ft=c
augroup END

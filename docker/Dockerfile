FROM kakadu18/ocaml:lama

RUN opam exec -- opam pin add Lama https://github.com/PLTools/Lama.git\#1.30 --no-action --yes
RUN opam exec -- opam install Lama --yes

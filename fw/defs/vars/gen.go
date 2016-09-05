package vars

//go:generate make -C .. vars.service.json
//go:generate clubbygen --input ../vars.service.json --lang go
//go:generate clubbygen --input ../vars.service.json --lang go --strict

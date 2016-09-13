package vars

//go:generate make -C .. vars.service.json
//go:generate miot clubbygen --input ../vars.service.json --lang go
//go:generate miot clubbygen --input ../vars.service.json --lang go --strict

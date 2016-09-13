package config

//go:generate make -C .. config.service.json
//go:generate miot clubbygen --input ../config.service.json --lang go
//go:generate miot clubbygen --input ../config.service.json --lang go --strict

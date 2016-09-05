package config

//go:generate make -C .. config.service.json
//go:generate clubbygen --input ../config.service.json --lang go
//go:generate clubbygen --input ../config.service.json --lang go --strict

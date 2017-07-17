package atca

import (
	"crypto"
	"crypto/ecdsa"
	"crypto/elliptic"
	"encoding/asn1"
	"encoding/base64"
	"fmt"
	"golang.org/x/net/context"
	"io"
	"math/big"
	"os"

	"cesanta.com/common/go/lptr"
	atcaService "cesanta.com/fw/defs/atca"
	"github.com/cesanta/errors"
)

// Implements crypto.Signer interface using ATCA.
type Signer struct {
	ctx  context.Context
	cl   atcaService.Service
	slot int
}

func NewSigner(ctx context.Context, cl atcaService.Service, slot int) crypto.Signer {
	return &Signer{ctx: ctx, cl: cl, slot: slot}
}

func (s *Signer) Public() crypto.PublicKey {
	pubk := &ecdsa.PublicKey{
		Curve: elliptic.P256(),
		X:     big.NewInt(0),
		Y:     big.NewInt(0),
	}

	req := &atcaService.GetPubKeyArgs{Slot: lptr.Int64(int64(s.slot))}

	r, err := s.cl.GetPubKey(s.ctx, req)
	if err != nil {
		//errors.Annotatef(err, "GetPubKey")
		return nil
	}

	keyData, _ := base64.StdEncoding.DecodeString(*r.Pubkey)

	pubk.X.SetBytes(keyData[:PublicKeySize/2])
	pubk.Y.SetBytes(keyData[PublicKeySize/2 : PublicKeySize])

	return pubk
}

type ecdsaSignature struct {
	R, S *big.Int
}

func (s *Signer) Sign(rand io.Reader, digest []byte, opts crypto.SignerOpts) ([]byte, error) {
	if len(digest) != 32 {
		return nil, errors.NotImplementedf("can only sign 32 byte digests, signing %d bytes", len(digest))
	}

	b64d := base64.StdEncoding.EncodeToString(digest)
	req := &atcaService.SignArgs{
		Slot:   lptr.Int64(int64(s.slot)),
		Digest: &b64d,
	}

	fmt.Fprintf(os.Stderr, "Signing with slot %d...\n", s.slot)

	r, err := s.cl.Sign(s.ctx, req)
	if err != nil {
		return nil, errors.Annotatef(err, "ATCA.Sign")
	}

	if r.Signature == nil {
		return nil, errors.New("no signature in response")
	}

	rawSig, _ := base64.StdEncoding.DecodeString(*r.Signature)
	if len(rawSig) != SignatureSize {
		return nil, errors.Errorf("invalid signature size: expected %d bytes, got %d",
			SignatureSize, len(rawSig))
	}

	sig := ecdsaSignature{
		R: big.NewInt(0),
		S: big.NewInt(0),
	}

	sig.R.SetBytes(rawSig[:SignatureSize/2])
	sig.S.SetBytes(rawSig[SignatureSize/2 : SignatureSize])

	return asn1.Marshal(sig)
}

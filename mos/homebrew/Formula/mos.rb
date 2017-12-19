class Mos < Formula
  include Language::Python::Virtualenv

  desc "Mongoose OS command-line tool"
  homepage "https://mongoose-os.com/"
  url "https://github.com/cesanta/mongoose-os/archive/1.22.1.tar.gz"
  sha256 "0edff29a6d3b67ee6e7868071be6bb96aab270af4ea4016f19b23ae13720043f"
  head ""

  depends_on "go" => :build
  depends_on "govendor" => :build
  depends_on "libftdi" => :build
  depends_on "libusb" => :build
  depends_on "libusb-compat" => :build
  depends_on "pkg-config" => :build
  depends_on "python" => :build if MacOS.version <= :snow_leopard

  # GitPython and its deps {{{
  resource "smmap2" do
    url "https://files.pythonhosted.org/packages/source/s/smmap2/smmap2-2.0.3.tar.gz"
    sha256 "c7530db63f15f09f8251094b22091298e82bf6c699a6b8344aaaef3f2e1276c3"
  end

  resource "gitdb2" do
    url "https://files.pythonhosted.org/packages/source/g/gitdb2/gitdb2-2.0.3.tar.gz"
    sha256 "b60e29d4533e5e25bb50b7678bbc187c8f6bcff1344b4f293b2ba55c85795f09"
  end

  resource "GitPython" do
    url "https://files.pythonhosted.org/packages/source/G/GitPython/GitPython-2.1.8.tar.gz"
    sha256 "ad61bc25deadb535b047684d06f3654c001d9415e1971e51c9c20f5b510076e9"
  end
  # }}}

  def install
    ENV["GOPATH"] = buildpath
    ENV["GOOS"] = "darwin"
    ENV["GOARCH"] = "amd64"
    ENV["CGO_ENABLED"] = "1"
    ENV["PATH"] += ":#{ENV["GOPATH"]}/bin"

    # Create virtualenv and install GitPython (with its deps) in it
    venv = virtualenv_create(libexec)
    %w[smmap2 gitdb2 GitPython].each do |r|
      venv.pip_install resource(r)
    end

    # Activate virtualenv for the rest of commands
    # (it is needed for "go generate ./...", since it involves running a python
    # script which uses GitPython)
    ENV["VIRTUAL_ENV"] = libexec
    ENV.delete("PYTHONHOME")
    ENV["PATH"] = "#{libexec}/bin:#{ENV["PATH"]}"

    path = buildpath/"src/cesanta.com"
    path.install Dir["{*,.git}"]

    cd path/"mos" do
      # The build will be performed not from a git repo, so we have to specify
      # version and build id manually. Use "brew" as a distro name so that mos
      # won't update itself.
      build_id = format("%s+???~brew", version)
      File.open(path/"mos/version/version", "w") { |file| file.write(version) }
      File.open(path/"mos/version/build_id", "w") { |file| file.write(build_id) }

      system "govendor", "sync"
      system "go", "install", "cesanta.com/vendor/github.com/jteeuwen/go-bindata/go-bindata"
      system "go", "install", "cesanta.com/vendor/github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs"
      system "go", "generate", "./..."
      system "go", "build", "-o", bin/"mos"
      prefix.install_metafiles
    end
  end

  test do
    system bin/"mos", "--version"
  end
end

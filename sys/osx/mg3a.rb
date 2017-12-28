class Mg3a < Formula
  desc "Small Emacs-like editor inspired by mg with UTF8 support."
  homepage "https://gtihub.com/paaguti/mg3a/"

  url "https://github.com/paaguti/mg3a/archive/171227.tar.gz"
  sha256 "82eb7e74b50756d5a7ddfc40b5368c0c097382b71fe2dd997001c958ec0e6e65"

  option "with-all",  "Include all fancy stuff (build with -DALL)"

  conflicts_with "mg", :because => "both install `mg`"

  patch do
    url "https://raw.githubusercontent.com/paaguti/mg3a/171227/debian/patches/01-pipein.diff"
    sha256 "e7cbf41772d9f748bc54978846342d1309bc35810cf6a2b73d4dce4b9c501a5b"
  end

  patch do
    url "https://raw.githubusercontent.com/paaguti/mg3a/171227/debian/patches/02-Makefile.bsd"
    sha256 "95d59dd1dbd5556b5309cba9c2b33364c4588579db45e9c7f98e380360dd4000"
  end

  def install
    if build.with?("all")
      mg3aopts = %w[DALL]
    else
      mg3aopts = %w[-DALL -DNO_UCSN -DNO_CMODE]
    end

    system "make", "CDEFS=#{mg3aopts * " "}", "LIBS=-lncurses", "COPT=-O3", "mg", "strip"
    bin.install "mg"
    doc.install Dir["bl/dot.*"]
    doc.install Dir["README*"]
  end

  test do
    (testpath/"command.sh").write <<~EOS
      #!/usr/bin/expect -f
      set timeout -1
      spawn #{bin}/mg
      match_max 100000
      send -- "\u0018\u0003"
      expect eof
    EOS
    (testpath/"command.sh").chmod 0755

    system testpath/"command.sh"
  end
end

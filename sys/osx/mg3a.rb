class Mg3a < Formula
  desc "Small Emacs-like editor inspired by mg with UTF8 support."
  homepage "https://gtihub.com/paaguti/mg3a/"

  url "https://github.com/paaguti/mg3a/archive/180101.tar.gz"
  # sha256 "82eb7e74b50756d5a7ddfc40b5368c0c097382b71fe2dd997001c958ec0e6e65"

  option "with-full",  "Include all fancy stuff (build with -DALL)"

  conflicts_with "mg", :because => "both install `mg`"

  def install
    if build.with?("full")
      mg3aopts = %w[full]
    else
      mg3aopts = %w[all]
    end

    system "make", "LIBS=-lncurses", "COPT=-O3", "#{mg3aopts * " "}", "strip"
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

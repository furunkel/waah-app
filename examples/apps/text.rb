class ExampleApp < Waah::App
  PLACEHOLDER = "Edit me..."

  def setup
    rate 60.0

    @lines = []
    keyboard.text do |t|
      log :info, "<#{t}> #{t.size}"
      @lines.push "" if @lines.empty?
      if t == "\n"
        @lines.push ""
      elsif t == "\b"
        if @lines.last.size <= 1
          @lines.pop
        else
          @lines[-1] = @lines.last[0...-1]
        end
      else
        @lines.last << t
      end
    end

    keyboard.show
  end

  def draw
    clear

    font_size 20

    if @lines.empty?
      font 'Sans', :italic
      text 40, 40, PLACEHOLDER
      color 0x90, 0x90, 0x90
      fill
    else
      font 'Sans'

      @lines.each_with_index do |line, index|
        text 40, 40 + index * 22, line
        color 0xff, 0xff, 0xff
        fill
      end
    end
  end
end

app = ExampleApp.new 500, 500, "Text Example"
app.run

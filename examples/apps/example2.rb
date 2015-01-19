class ExampleApp < Waah::App
  def setup
    rate 60.0
    log_level :verbose

    @lines = [""]
    keyboard.text do |t|

      if t == "\n"
        @lines.push ""
      elsif t == "\b"
        if @lines.last.empty?
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
    @lines.each_with_index do |line, index|
      text 40, 40 + index * 22, line
      color 0xff, 0xff, 0xff
      fill
    end

    redraw
  end
end

app = ExampleApp.new 500, 500, "Example 2"
app.run

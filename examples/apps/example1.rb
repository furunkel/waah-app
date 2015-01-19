class ExampleApp < Waah::App
  def setup
    rate 10.0
    log_level :verbose

    log :verbose, "Running app"

    @text = ""
    keyboard.text do |t|
      log :verbose, "texttt: #{t}"
      @text << t
    end
  end

  def draw
    clear

    @r ||= 0
    translate 100, 100 do
      rotate((@r % 100) / 100 * 2 * 3.14) do
        rounded_rect 0, 0, 50, 50, 3
      end
    end
    @r += 5

    if pointer.pressed? && pointer.in?
      color 0, 0xff, 0
      keyboard.toggle
    else
      color 0xff, 0, 0
    end
    fill true
    color 0, 0xff, 0
    stroke

    font_size 20
    text 200, 100, @text
    color 0, 0, 0xff
    fill

    redraw
  end
end

app = ExampleApp.new 500, 500, "Example App"

begin
  app.run
rescue Exception => e
  app.log :error, e.inspect
end

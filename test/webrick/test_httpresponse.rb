require "webrick"
require "minitest/autorun"
require "stringio"
require "net/http"

module WEBrick
  class TestHTTPResponse < MiniTest::Unit::TestCase
    class FakeLogger
      attr_reader :messages

      def initialize
        @messages = []
      end

      def warn msg
        @messages << msg
      end
    end

    attr_reader :config, :logger, :res

    def setup
      super
      @logger          = FakeLogger.new
      @config          = Config::HTTP
      @config[:Logger] = logger
      @res             = HTTPResponse.new config
      @res.keep_alive  = true
    end

    def test_prevent_response_splitting_headers
      res['X-header'] = "malicious\r\nCookie: hack"
      io = StringIO.new
      res.send_response io
      io.rewind
      res = Net::HTTPResponse.read_new(Net::BufferedIO.new(io))
      assert_equal '500', res.code
      refute_match 'hack', io.string
    end

    def test_prevent_response_splitting_cookie_headers
      user_input = "malicious\r\nCookie: hack"
      res.cookies << WEBrick::Cookie.new('author', user_input)
      io = StringIO.new
      res.send_response io
      io.rewind
      res = Net::HTTPResponse.read_new(Net::BufferedIO.new(io))
      assert_equal '500', res.code
      refute_match 'hack', io.string
    end

    def test_304_does_not_log_warning
      res.status      = 304
      res.setup_header
      assert_equal 0, logger.messages.length
    end

    def test_204_does_not_log_warning
      res.status      = 204
      res.setup_header

      assert_equal 0, logger.messages.length
    end

    def test_1xx_does_not_log_warnings
      res.status      = 105
      res.setup_header

      assert_equal 0, logger.messages.length
    end
  end
end

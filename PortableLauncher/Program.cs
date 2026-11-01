using System.Diagnostics;
using System.IO.Compression;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

internal static class Program
{
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int MessageBox(IntPtr hwnd, string text, string caption, uint type);

    [STAThread]
    private static int Main(string[] args)
    {
        try
        {
            using var payload = Assembly.GetExecutingAssembly().GetManifestResourceStream("Game.zip")
                ?? throw new InvalidDataException("The game payload is missing.");
            string version = Convert.ToHexString(SHA256.HashData(payload))[..16];
            payload.Position = 0;
            string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Mannetroll", "GotlandRingUnreal");
            string destination = Path.Combine(root, version);
            using var mutex = new Mutex(false, "Local\\MannetrollGotlandRingUnreal" + version);
            bool held = false;
            try
            {
                try { held = mutex.WaitOne(TimeSpan.FromMinutes(3)); }
                catch (AbandonedMutexException) { held = true; }
                if (!held)
                    throw new IOException("Another copy is preparing the game. Please try again shortly.");
                if (!File.Exists(Path.Combine(destination, "ready.txt")))
                {
                    Directory.CreateDirectory(root);
                    string staging = Path.Combine(root, version + "-" + Guid.NewGuid().ToString("N"));
                    Directory.CreateDirectory(staging);
                    try
                    {
                        using (var archive = new ZipArchive(payload, ZipArchiveMode.Read, true))
                            archive.ExtractToDirectory(staging);
                        string game = Path.Combine(staging, "GotlandRingUnreal");
                        if (!File.Exists(Path.Combine(staging, "GotlandRingUnreal.exe"))
                            || !File.Exists(Path.Combine(game, "Binaries", "Win64", "GotlandRingUnreal-Win64-Shipping.exe"))
                            || !File.Exists(Path.Combine(game, "Content", "Paks", "GotlandRingUnreal-Windows.utoc"))
                            || !File.Exists(Path.Combine(game, "Content", "Paks", "GotlandRingUnreal-Windows.ucas")))
                            throw new InvalidDataException("The extracted game is incomplete.");
                        File.WriteAllText(Path.Combine(staging, "ready.txt"), version);
                        Directory.Move(staging, destination);
                    }
                    finally
                    {
                        if (Directory.Exists(staging)) Directory.Delete(staging, true);
                    }
                }
            }
            finally
            {
                if (held) mutex.ReleaseMutex();
            }
            if (args.Contains("--extract-only")) return 0;
            var start = new ProcessStartInfo(Path.Combine(destination, "GotlandRingUnreal.exe"))
            {
                WorkingDirectory = destination,
                UseShellExecute = false
            };
            foreach (string argument in args) start.ArgumentList.Add(argument);
            _ = Process.Start(start) ?? throw new IOException("Windows could not start the game.");
            return 0;
        }
        catch (Exception error)
        {
            MessageBox(IntPtr.Zero, error.Message, "Gotland Ring Unreal", 0x10);
            return 1;
        }
    }
}

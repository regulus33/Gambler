# Gambler

```
┌─────────────────────────────────────┐
│  Presentation Layer (UI Thread)     │
│  - JUCE Components                  │
│  - Button/Slider listeners          │
└──────────────┬──────────────────────┘
               │ Commands/Queries
┌──────────────▼──────────────────────┐
│  Application Layer                  │
│  - AudioProcessor (Controller)      │
│  - Command handlers                 │
│  - APVTS (persistence adapter)      │
└──────────────┬──────────────────────┘
               │ Method calls
┌──────────────▼──────────────────────┐
│  Domain Layer (Thread-safe)         │
│  - Controls (aggregate root)        │
│  - Sequence, Note, etc.             │
│  - Pure C++, no JUCE dependencies   │
└─────────────────────────────────────┘djjj
```